#include "icmp.h"
#include "ip.h"
#include "net_utils.h"
#include <stdio.h>
#include <string.h>

void icmp_input(const uint8_t *payload, size_t len, uint32_t src_ip, uint32_t dst_ip) {
    if (len < sizeof(struct icmp_hdr)) {
        printf("ICMP: packet too short\n");
        return;
    }

    const struct icmp_hdr *icmp = (const struct icmp_hdr *)payload;

    switch (icmp->type) {
        case ICMP_ECHO_REQUEST: {
            printf("ICMP: received Echo Request from 0x%08X\n", src_ip);
            uint8_t reply[2048];
            if (len > sizeof(reply)) {
                printf("ICMP: echo request too large\n");
                return;
            }
            memcpy(reply, payload, len);
            struct icmp_hdr *reply_hdr = (struct icmp_hdr *)reply;
            reply_hdr->type = ICMP_ECHO_REPLY;
            reply_hdr->code = 0;
            reply_hdr->checksum = 0;
            reply_hdr->checksum = ip_checksum(reply, len);

            ip_output(reply, len, dst_ip, src_ip, IP_PROTO_ICMP);
            printf("ICMP: sent Echo Reply to 0x%08X\n", src_ip);
            break;
        }
        case ICMP_ECHO_REPLY:
            printf("ICMP: received Echo Reply\n");
            break;
        default:
            printf("ICMP: unhandled type %d\n", icmp->type);
            break;
    }
}
