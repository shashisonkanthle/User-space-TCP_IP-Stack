#include "udp.h"
#include "ip.h"
#include "net_utils.h"
#include <stdio.h>
#include <string.h>

void udp_input(const uint8_t *payload, size_t len, uint32_t src_ip, uint32_t dst_ip) {
    if (len < sizeof(struct udp_hdr)) {
        printf("UDP: packet too short\n");
        return;
    }

    const struct udp_hdr *udp = (const struct udp_hdr *)payload;
    uint16_t src_port = my_ntohs(udp->src_port);
    uint16_t dst_port = my_ntohs(udp->dst_port);
    uint16_t udp_len = my_ntohs(udp->len);
    uint16_t checksum = my_ntohs(udp->checksum);

    if (udp_len != len) {
        printf("UDP: length mismatch (%d vs %zu)\n", udp_len, len);
        return;
    }

    if (checksum != 0) {
        struct udp_pseudo_hdr pseudo;
        pseudo.src_addr = src_ip;
        pseudo.dst_addr = dst_ip;
        pseudo.zero = 0;
        pseudo.protocol = IP_PROTO_UDP;
        pseudo.udp_len = my_htons(len);

        uint8_t check_buf[2048];
        memcpy(check_buf, &pseudo, sizeof(pseudo));
        memcpy(check_buf + sizeof(pseudo), payload, len);
        uint16_t computed = ip_checksum(check_buf, sizeof(pseudo) + len);
        if (computed != 0) {
            printf("UDP: bad checksum\n");
            return;
        }
    }

    const uint8_t *data = payload + sizeof(struct udp_hdr);
    size_t data_len = len - sizeof(struct udp_hdr);
    printf("UDP: received %zu bytes from %d to port %d\n",
           data_len, src_port, dst_port);

    if (data_len > 0) {
        printf("UDP data: ");
        for (size_t i = 0; i < data_len && i < 100; i++) {
            printf("%c", data[i]);
        }
        printf("\n");
    }
}
