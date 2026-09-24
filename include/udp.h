#ifndef UDP_H
#define UDP_H
#include <stdint.h>
#include <stddef.h>

struct udp_hdr {
    uint16_t src_port; 
    uint16_t dst_port; 
    uint16_t len;// total length of UDP header and data   
    uint16_t checksum;  
} __attribute__((packed));

/* This prevents misdelivery to the wrong IP address. */
struct udp_pseudo_hdr {
    uint32_t src_addr;  
    uint32_t dst_addr; 
    uint8_t zero;   
    uint8_t protocol;   
    uint16_t udp_len;   
} __attribute__((packed));

static inline void udp_input(const uint8_t *payload, size_t len,
                             uint32_t src_ip, uint32_t dst_ip);
    

#endif