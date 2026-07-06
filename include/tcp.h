#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stdbool.h>

struct tcp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_off;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
} tcp_state_t;

#define TCP_BUF_SIZE 65536

struct tcp_pcb {

    struct tcp_pcb *next;
    struct tcp_pcb *prev;

    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;

    tcp_state_t state;

    uint32_t snd_una;
    uint32_t snd_nxt;
    uint32_t snd_max;
    uint32_t iss;

    uint32_t rcv_nxt;
    uint32_t rcv_wnd;
    uint32_t irs;

    uint8_t send_buf[TCP_BUF_SIZE];
    uint32_t send_buf_len;
    uint32_t send_buf_head;

    uint8_t recv_buf[TCP_BUF_SIZE];
    uint32_t recv_buf_len;
    uint32_t recv_buf_head;

    uint32_t rto;
    uint32_t srtt;
    uint32_t rttvar;
    uint32_t retrans_count;

    uint32_t cwnd;
    uint32_t ssthresh;

    bool pending_close;
};

#endif