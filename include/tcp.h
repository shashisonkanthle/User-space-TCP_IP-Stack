#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stdbool.h> 
#include <stddef.h> // for size_t

struct tcp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_off; // 4 bits data offset in 32-bit units + 4 bits reserved
    uint8_t flags;// 6 bits flags used for control like SYN, ACK, RST etc. + 2 bits reserved
    uint16_t window;// size of the receive window
    uint16_t checksum;
    uint16_t urgent;// pointer to urgent data
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
    
    //sending pipeline
    uint32_t snd_una;// oldest unacknowledged sequence number
    uint32_t snd_nxt;// next sequence number to be sent
    uint32_t snd_max;// highest sequence number sent so far
    uint32_t iss; // initial send sequence number
    
    //receiving pipeline
    uint32_t rcv_nxt; //next seq number expected to be received
    uint32_t rcv_wnd;// size of the receive window
    uint32_t irs; // initial receive sequence number

    uint8_t send_buf[TCP_BUF_SIZE];
    uint32_t send_buf_len;// total number of bytes in the send buffer
    uint32_t send_buf_head;// index of the first byte in the send buffer that has not been acknowledged yet

    uint8_t recv_buf[TCP_BUF_SIZE];
    uint32_t recv_buf_len;// total number of bytes in the receive buffer
    uint32_t recv_buf_head; // index of the first byte in the receive buffer that has not been read yet

    uint32_t rto; // retransmission timeout in milliseconds
    uint32_t srtt; // smoothed round-trip time in milliseconds
    uint32_t rttvar; // round-trip time variation in milliseconds
    uint32_t retrans_count; // number of retransmissions for the current segment

    uint32_t cwnd; // congestion window size in bytes who much data can be sent before receiving an ACK
    uint32_t ssthresh; // slow start threshold in bytes, linear growth of the congestion window after reaching this threshold

    bool pending_close; // flag indicating whether the connection is pending closure
};

static inline void tcp_input(const uint8_t *payload, size_t len,
                             uint32_t src_ip, uint32_t dst_ip) {
    (void)payload; // void to avoid unused parameter warning
    (void)len;
    (void)src_ip;
    (void)dst_ip;
}

#endif