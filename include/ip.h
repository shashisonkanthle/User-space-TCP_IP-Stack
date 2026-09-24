#ifndef IP_H
#define IP_H
#include <stdint.h>
#include <stddef.h>

struct ip_hdr {
    uint8_t ihl_version; // 4 bits version + 4 bits IHL (Internet Header Length)
    uint8_t tos; // Type of Service 
    uint16_t tot_len; 
    uint16_t id; // Identification
    uint16_t frag_off; // Fragment Offset
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

void ip_input(const uint8_t *packet, size_t len);
void ip_output(const uint8_t *payload, size_t payload_len,
               uint32_t src, uint32_t dst, uint8_t proto);
#endif