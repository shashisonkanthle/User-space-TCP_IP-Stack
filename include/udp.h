#ifndef UDP_H
#define UDP_H
#include <stdint.h>

struct udp_hdr {
    uint16_t src_port; 
    uint16_t dst_port; 
    uint16_t len;   
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
#endif