#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include "tun.h"
#include "ip.h"
#include "net_utils.h"

#define BUF_SIZE 2048

int g_tun_fd;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    char tun_name[16] = "tun0";
    const char *host_tun_ip = "192.168.10.2";
    uint8_t buf[BUF_SIZE];
    int n;

    g_tun_fd = tun_create(tun_name, host_tun_ip, 24);
    if (g_tun_fd < 0) {
        fprintf(stderr, "Failed to create TUN device\n");
        return 1;
    }
    printf("Stack running on %s (fd=%d)\n", tun_name, g_tun_fd);
    printf("Host side IP on %s: %s\n", tun_name, host_tun_ip);
    printf("Stack endpoint IP: 192.168.10.1\n");
    printf("Try: ping 192.168.10.1\n");

    while (1) {
        n = tun_read(g_tun_fd, buf, BUF_SIZE);
        if (n < 0) {
            perror("tun_read");
            break;
        }
        if (n == 0) {
            printf("TUN device closed\n");
            break;
        }
        ip_input(buf, n);
    }
    close(g_tun_fd);
    return 0;
}
