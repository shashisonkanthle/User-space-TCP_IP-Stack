#ifndef ICMP_H
#define ICMP_H
#include <stdint.h>
#include <stddef.h>

struct icmp_hdr {
    uint8_t type;   
    uint8_t code;   
    uint16_t checksum;  
    uint16_t id;    
    uint16_t seq;   
} __attribute__((packed));

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

void icmp_input(const uint8_t *payload, size_t len, uint32_t src_ip, uint32_t dst_ip);
#endif