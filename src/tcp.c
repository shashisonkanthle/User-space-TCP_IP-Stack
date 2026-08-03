#include "tcp.h"
#include "ip.h"
#include "net_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TCP_MSS 1460

struct tcp_pcb *tcp_pcbs = NULL;

struct tcp_pcb *tcp_new_pcb(void) {
    struct tcp_pcb *pcb = calloc(1, sizeof(struct tcp_pcb));
    if (!pcb) {
        perror("tcp_new_pcb: calloc");
        return NULL;
    }
    pcb->state = TCP_CLOSED;
    pcb->cwnd = TCP_MSS;
    pcb->ssthresh = 65535;
    pcb->rcv_wnd = 65535;
    pcb->next = tcp_pcbs;
    tcp_pcbs = pcb;
    return pcb;
}

struct tcp_pcb *tcp_find_pcb(uint32_t local_ip, uint16_t local_port,
                             uint32_t remote_ip, uint16_t remote_port) {
    struct tcp_pcb *pcb = tcp_pcbs;
    while (pcb) {
        if (pcb->local_ip == local_ip && pcb->local_port == local_port) {
            if (pcb->state == TCP_ESTABLISHED) {
                if (pcb->remote_ip == remote_ip && pcb->remote_port == remote_port) {
                    return pcb;
                }
            } else if (pcb->state == TCP_LISTEN) {
                return pcb;
            }
        }
        pcb = pcb->next;
    }
    return NULL;
}

void tcp_send_raw(struct tcp_pcb *pcb, uint8_t flags,
                  const uint8_t *data, size_t data_len) {
    uint8_t segment[2048];
    struct tcp_hdr *tcp = (struct tcp_hdr *)segment;

    tcp->src_port = my_htons(pcb->local_port);
    tcp->dst_port = my_htons(pcb->remote_port);
    tcp->seq = my_htonl(pcb->snd_nxt);
    tcp->ack = my_htonl(pcb->rcv_nxt);
    tcp->data_off = (5 << 4);
    tcp->flags = flags;
    tcp->window = my_htons(pcb->rcv_wnd);
    tcp->checksum = 0;
    tcp->urgent = 0;

    if (data && data_len > 0) {
        memcpy(segment + 20, data, data_len);
        pcb->snd_nxt += data_len;
    }
    size_t seg_len = 20 + data_len;

    struct tcp_pseudo_hdr {
        uint32_t src;
        uint32_t dst;
        uint8_t zero;
        uint8_t proto;
        uint16_t len;
    } __attribute__((packed)) pseudo;
    pseudo.src = pcb->local_ip;
    pseudo.dst = pcb->remote_ip;
    pseudo.zero = 0;
    pseudo.proto = IP_PROTO_TCP;
    pseudo.len = my_htons(seg_len);

    uint8_t check_buf[4096];
    memcpy(check_buf, &pseudo, sizeof(pseudo));
    memcpy(check_buf + sizeof(pseudo), segment, seg_len);
    tcp->checksum = ip_checksum(check_buf, sizeof(pseudo) + seg_len);

    ip_output(segment, seg_len, pcb->local_ip, pcb->remote_ip, IP_PROTO_TCP);
    printf("TCP sent: flags=0x%02X seq=%u ack=%u len=%zu\n",
           flags, my_ntohl(tcp->seq), my_ntohl(tcp->ack), data_len);
}

void tcp_input(const uint8_t *payload, size_t len,
               uint32_t src_ip, uint32_t dst_ip) {
    if (len < 20) {
        printf("TCP: packet too short\n");
        return;
    }

    const struct tcp_hdr *tcp = (const struct tcp_hdr *)payload;
    uint16_t src_port = my_ntohs(tcp->src_port);
    uint16_t dst_port = my_ntohs(tcp->dst_port);
    uint32_t seq = my_ntohl(tcp->seq);
    uint32_t ack = my_ntohl(tcp->ack);
    uint8_t flags = tcp->flags;

    uint8_t hdr_len = (tcp->data_off >> 4) * 4;

    struct tcp_pcb *pcb = tcp_find_pcb(dst_ip, dst_port, src_ip, src_port);

    if (!pcb && (flags & TCP_SYN)) {// isolating the SYN bit for new connection requests
        pcb = tcp_find_pcb(dst_ip, dst_port, 0, 0);
        if (pcb && pcb->state == TCP_LISTEN) {
            struct tcp_pcb *new_pcb = tcp_new_pcb();
            if (!new_pcb) return;

            new_pcb->state = TCP_SYN_RECEIVED;
            new_pcb->local_ip = dst_ip;
            new_pcb->local_port = dst_port;
            new_pcb->remote_ip = src_ip;
            new_pcb->remote_port = src_port;
            new_pcb->irs = seq;
            new_pcb->rcv_nxt = seq + 1;

            static uint32_t isnc = 1000;
            new_pcb->iss = isnc++;
            new_pcb->snd_nxt = new_pcb->iss;
            new_pcb->snd_una = new_pcb->iss;

            tcp_send_raw(new_pcb, TCP_SYN | TCP_ACK, NULL, 0);
            new_pcb->snd_nxt++;// SYN | ACK act as virtual data, so we increment snd_nxt by 1, wait for ACK from client
            printf("TCP: SYN-ACK sent, state=SYN_RECEIVED\n");
            return;
        }
    }

    if (!pcb) {
        printf("TCP: no PCB for connection, sending RST\n");
        return;
    }

    switch (pcb->state) {
        case TCP_SYN_RECEIVED:
            if (flags & TCP_ACK) {
                if (ack == pcb->iss + 1) {
                    pcb->state = TCP_ESTABLISHED;
                    pcb->snd_una = ack;
                    printf("TCP: connection established!\n");
                }
            }
            break;

        case TCP_ESTABLISHED:
            if (flags & TCP_ACK) {
                if (ack > pcb->snd_una && ack <= pcb->snd_nxt) {
                    uint32_t acked = ack - pcb->snd_una;
                    pcb->snd_una = ack;
                    if (acked > 0 && pcb->send_buf_len >= acked) {
                        memmove(pcb->send_buf,
                                pcb->send_buf + acked,
                                pcb->send_buf_len - acked);
                        pcb->send_buf_len -= acked;
                    }
                    printf("TCP: ACKed %u bytes, snd_una=%u\n", acked, pcb->snd_una);
                }
            }

            size_t payload_len = len - hdr_len;
            if (payload_len > 0) {
                if (seq == pcb->rcv_nxt) {
                    if (pcb->recv_buf_len + payload_len <= TCP_BUF_SIZE) {
                        memcpy(pcb->recv_buf + pcb->recv_buf_len,
                               payload + hdr_len, payload_len);
                        pcb->recv_buf_len += payload_len;
                        pcb->rcv_nxt += payload_len;
                        tcp_send_raw(pcb, TCP_ACK, NULL, 0);
                        printf("TCP: received %zu bytes, rcv_nxt=%u\n", payload_len, pcb->rcv_nxt);
                    }
                } else {
                    printf("TCP: out-of-order seq (got %u, expected %u)\n", seq, pcb->rcv_nxt);
                }
            }

            if (flags & TCP_FIN) {
                printf("TCP: received FIN\n");
                pcb->rcv_nxt++;
                tcp_send_raw(pcb, TCP_ACK, NULL, 0);
                pcb->state = TCP_CLOSE_WAIT;
            }
            break;

        case TCP_CLOSE_WAIT:
            break;

        case TCP_LAST_ACK:
            if (flags & TCP_ACK) {
                if (ack == pcb->snd_nxt + 1) {
                    pcb->state = TCP_CLOSED;
                    printf("TCP: connection closed\n");
                }
            }
            break;

        default:
            printf("TCP: unhandled state %d\n", pcb->state);
            break;
    }
}

int tcp_send(struct tcp_pcb *pcb, const void *data, size_t len) {
    if (pcb->state != TCP_ESTABLISHED) {
        printf("TCP send: not established (state=%d)\n", pcb->state);
        return -1;
    }
    size_t avail = TCP_BUF_SIZE - pcb->send_buf_len;
    if (len > avail) {
        printf("TCP send: buffer full (avail=%zu, requested=%zu)\n", avail, len);
        len = avail;
    }
    if (len == 0) {
        return 0;
    }
    memcpy(pcb->send_buf + pcb->send_buf_len, data, len);
    pcb->send_buf_len += len;
    printf("TCP send: copied %zu bytes to send buffer\n", len);
    return len;
}

int tcp_recv(struct tcp_pcb *pcb, void *buf, size_t len) {
    if (pcb->recv_buf_len == 0) {
        return 0;
    }
    size_t to_copy = (len < pcb->recv_buf_len) ? len : pcb->recv_buf_len;
    memcpy(buf, pcb->recv_buf, to_copy);
    if (to_copy < pcb->recv_buf_len) {
        memmove(pcb->recv_buf, pcb->recv_buf + to_copy, pcb->recv_buf_len - to_copy);
    }
    pcb->recv_buf_len -= to_copy;
    printf("TCP recv: delivered %zu bytes to application\n", to_copy);
    return to_copy;
}

void tcp_output(struct tcp_pcb *pcb) {
    if (pcb->state != TCP_ESTABLISHED) {
        return;
    }
    uint32_t in_flight = pcb->snd_nxt - pcb->snd_una;
    uint32_t available = pcb->cwnd;
    if (in_flight >= available) {
        return;
    }
    uint32_t can_send = available - in_flight;
    uint32_t buf_avail = pcb->send_buf_len - (pcb->snd_nxt - pcb->snd_una);
    if (can_send > buf_avail) {
        can_send = buf_avail;
    }
    if (can_send > TCP_MSS) {
        can_send = TCP_MSS;
    }
    if (can_send == 0) {
        return;
    }
    uint32_t offset = pcb->snd_nxt - pcb->snd_una;
    tcp_send_raw(pcb, TCP_ACK, pcb->send_buf + offset, can_send);
}

void tcp_init_rto(struct tcp_pcb *pcb) {
    pcb->srtt = 0;
    pcb->rttvar = 250;
    pcb->rto = 1000; // after 1 second, retransmit if no ACK received
    pcb->retrans_count = 0; // number of retransmissions for the current segment
}

void tcp_update_rto(struct tcp_pcb *pcb, uint32_t rtt_sample) {
    if (pcb->srtt == 0) {
        pcb->srtt = rtt_sample;
        pcb->rttvar = rtt_sample / 2;
    } else {
        int32_t delta = (int32_t)pcb->srtt - (int32_t)rtt_sample;// signed difference
        if (delta < 0) delta = -delta;
        pcb->srtt = (7 * pcb->srtt + rtt_sample) / 8;// exponential weighted moving average
        pcb->rttvar = (3 * pcb->rttvar + delta) / 4;
    }
    pcb->rto = pcb->srtt + 4 * pcb->rttvar;
    if (pcb->rto < 200) pcb->rto = 200;
    if (pcb->rto > 64000) pcb->rto = 64000;
    printf("TCP RTO updated: srtt=%u rttvar=%u rto=%u\n", pcb->srtt, pcb->rttvar, pcb->rto);
}

void tcp_handle_ack(struct tcp_pcb *pcb, uint32_t ack) {
    if (ack <= pcb->snd_una) {
        return;// old ACK, ignore
    }
    pcb->snd_una = ack;
    pcb->retrans_count = 0;// reset retransmission count on successful ACK
    if (pcb->cwnd < pcb->ssthresh) {
        pcb->cwnd += TCP_MSS;
        printf("TCP Slow Start: cwnd=%u (ssthresh=%u)\n", pcb->cwnd, pcb->ssthresh);
    } else {
        pcb->cwnd += (TCP_MSS * TCP_MSS) / pcb->cwnd;
        printf("TCP Congestion Avoidance: cwnd=%u\n", pcb->cwnd);
    }
}

void tcp_handle_timeout(struct tcp_pcb *pcb) {
    printf("TCP: RTO expired, retransmitting\n");
    pcb->ssthresh = pcb->cwnd / 2;
    if (pcb->ssthresh < 2 * TCP_MSS) {
        pcb->ssthresh = 2 * TCP_MSS;
    }
    pcb->cwnd = TCP_MSS;
    pcb->rto *= 2;
    if (pcb->rto > 64000) {
        pcb->rto = 64000;
    }
    uint32_t offset = 0;
    uint32_t to_send = pcb->send_buf_len - offset;
    if (to_send > TCP_MSS) {
        to_send = TCP_MSS;
    }
    if (to_send > 0) {
        pcb->snd_nxt = pcb->snd_una;
        tcp_send_raw(pcb, TCP_ACK, pcb->send_buf, to_send);
        pcb->retrans_count++;
        printf("TCP: retransmitted %u bytes (attempt %u)\n", to_send, pcb->retrans_count);
    }
}
