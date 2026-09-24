#include "ip.h"
#include "net_utils.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "tun.h"
#include <stdio.h>
#include <string.h>

static const uint32_t tun_ip_host = 0xC0A80A01u;//192.168.10.1

extern int g_tun_fd;

void ip_input(const uint8_t *packet, size_t len) {
    if (len < sizeof(struct ip_hdr)) {
        return;
    }

    const struct ip_hdr *ip = (const struct ip_hdr *)packet;

    uint8_t version = (ip->ihl_version >> 4) & 0x0F;
    if (version != 4) {
        return;
    }

    uint8_t ihl = (ip->ihl_version & 0x0F) * 4;
    if (ihl < 20) {
        printf("IP: invalid IHL (%d)\n", ihl);
        return;
    }

    uint16_t tot_len = my_ntohs(ip->tot_len);
    if (tot_len < ihl || tot_len > len) {
        printf("IP: invalid total length (%d vs %zu)\n", tot_len, len);
        return;
    }

    uint8_t hdr_copy[60];
    memcpy(hdr_copy, packet, ihl);
    struct ip_hdr *ip_copy = (struct ip_hdr *)hdr_copy;
    uint16_t received_check = ip_copy->check;
    ip_copy->check = 0;
    uint16_t computed = ip_checksum(hdr_copy, ihl);
    if (computed != received_check) {// for debugging if not ip_checksum(ip,ihl) != 0       
            printf("IP: bad checksum (computed=0x%04X, received=0x%04X)\n",
                   my_ntohs(computed), my_ntohs(received_check));
        return;
    }

    if (ip->daddr != my_htonl(tun_ip_host)) {
        return;
    }

    const uint8_t *payload = packet + ihl;
    size_t payload_len = tot_len - ihl;

    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            icmp_input(payload, payload_len, ip->saddr, ip->daddr);
            break;
        case IP_PROTO_UDP:
            break;
        case IP_PROTO_TCP:
            break;
        default:
            break;
    }
}

void ip_output(const uint8_t *payload, size_t payload_len,
               uint32_t src, uint32_t dst, uint8_t proto) {
    uint8_t packet[2048];
    size_t total_len = 20 + payload_len;
    if (total_len > sizeof(packet)) {
        printf("IP: packet too large (%zu)\n", total_len);
        return;
    }

    struct ip_hdr *ip = (struct ip_hdr *)packet;
    ip->ihl_version = (4 << 4) | 5;
    ip->tos = 0;
    ip->tot_len = my_htons(total_len);

    static uint16_t ip_id = 0;
    ip->id = my_htons(ip_id++);
    ip->frag_off = my_htons(0x4000);
    ip->ttl = 64;
    ip->protocol = proto;
    ip->check = 0;
    ip->saddr = src;
    ip->daddr = dst;

    ip->check = ip_checksum(ip, 20);
    memcpy(packet + 20, payload, payload_len);

    int n = tun_write(g_tun_fd, packet, total_len);
    if (n < 0) {
        perror("ip_output: tun_write");
    }
}
