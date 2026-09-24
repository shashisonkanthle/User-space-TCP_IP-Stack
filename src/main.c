#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <net/if.h>
#include "tun.h"
#include "ip.h"
#include "net_utils.h"
#include "http_server.h"

#define BUF_SIZE 2048

int g_tun_fd;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    char tun_name[IFNAMSIZ] = "tun0";
    const char *host_tun_ip = "192.168.10.2";
    uint8_t buf[BUF_SIZE];
    int n;

    /* Create and configure TUN interface (host IP 192.168.10.2, stack IP 192.168.10.1) */
    g_tun_fd = tun_create(tun_name, host_tun_ip, 24);
    if (g_tun_fd < 0) {
        fprintf(stderr, "Failed to create TUN device\n");
        return 1;
    }
    printf("Stack running on %s (fd=%d)\n", tun_name, g_tun_fd);
    printf("Host side IP on %s: %s\n", tun_name, host_tun_ip);
    printf("Stack endpoint IP: 192.168.10.1\n");

    /* Spawn HTTP server in a detached background thread */
    pthread_t http_tid;
    if (pthread_create(&http_tid, NULL, http_server_thread, NULL) != 0) {
        perror("pthread_create http_server");
        close(g_tun_fd);
        return 1;
    }
    pthread_detach(http_tid);

    printf("Try: ping 192.168.10.1 or curl http://192.168.10.1\n");

    /* Main thread runs the TUN RX packet processing loop */
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
