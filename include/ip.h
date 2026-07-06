#ifndef IP_H
#define IP_H
#include <stdint.h>

struct ip_hdr {
    uint8_t ihl_version; 
    uint8_t tos; 
    uint16_t tot_len; 
    uint16_t id; 
    uint16_t frag_off; 
    uint8_t ttl; 
    uint8_t protocol; 
    uint16_t check; 
    uint32_t saddr; 
    uint32_t daddr;
 } __attribute__((packed));
/* Protocol numbers (from RFC 791) */
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17
#endif