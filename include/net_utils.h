#ifndef NET_UTILS_H
#define NET_UTILS_H
#include <stdint.h>
#include <stddef.h>

static inline uint16_t my_htons(uint16_t host) {
    return ((host & 0x00FF) << 8) | 
           ((host & 0xFF00) >> 8); 
}

static inline uint32_t my_htonl(uint32_t host) {
    return ((host & 0x000000FF) << 24) |
           ((host & 0x0000FF00) << 8) |
           ((host & 0x00FF0000) >> 8) |
           ((host & 0xFF000000) >> 24);
}

static inline uint16_t my_ntohs(uint16_t net) {
    return my_htons(net); 
}

static inline uint32_t my_ntohl(uint32_t net) {
    return my_htonl(net); 
}

static inline uint16_t ip_checksum(const void *data, size_t len) {
    const uint16_t *buf = (const uint16_t *)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *buf++; len -= 2;}
    if (len == 1) 
        sum += *(const uint8_t *)buf;

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16); }

    return (uint16_t)(~sum); 
}
#endif