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

#define BUF_SIZE 2048

int g_tun_fd;

extern int http_server_run(void);

static void *tun_rx_thread(void *arg) {
    (void)arg;
    uint8_t buf[BUF_SIZE];
    int n;

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
    return NULL;
}

int main(void) {
    char tun_name[16] = "tun0";
    const char *host_tun_ip = "192.168.10.2";
    /* This is the IP address assigned to TUN device on host OS itself
     * & this IP belongs to the Linux kernel's native network stack */

    g_tun_fd = tun_create(tun_name, host_tun_ip, 24);
    if (g_tun_fd < 0) {
        fprintf(stderr, "Failed to create TUN device\n");
        return 1;
    }
    printf("Stack running on %s (fd=%d)\n", tun_name, g_tun_fd);
    printf("Host side IP on %s: %s\n", tun_name, host_tun_ip);
    printf("Stack endpoint IP: 192.168.10.1\n");
    printf("Try: ping 192.168.10.1 or curl http://192.168.10.1\n");

    pthread_t rx_thread;
    if (pthread_create(&rx_thread, NULL, tun_rx_thread, NULL) != 0) {
        perror("pthread_create");
        close(g_tun_fd);
        return 1;
    }

    /* Run the HTTP server application */
    http_server_run();

    pthread_join(rx_thread, NULL);
    close(g_tun_fd);
    return 0;
}
