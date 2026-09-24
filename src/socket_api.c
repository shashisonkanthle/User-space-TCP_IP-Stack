#include "socket_api.h"
#include "tcp.h"
#include "net_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* === SOCKET TABLE === */
#define MAX_SOCKETS 128

struct socket_entry {
    struct tcp_pcb *pcb; /* NULL = free slot */
};

static struct socket_entry sockets[MAX_SOCKETS];

/* === INTERNAL HELPERS === */
static int find_free_socket(void) {
    for (int i = 1; i < MAX_SOCKETS; i++) {
        if (sockets[i].pcb == NULL) {
            return i;
        }
    }
    return -1;
}

int t_socket(int domain, int type, int protocol) {
    (void)protocol;
    if (domain != AF_INET) {
        printf("t_socket: only AF_INET supported\n");
        return -1;
    }
    if (type != SOCK_STREAM) {
        printf("t_socket: only SOCK_STREAM supported\n");
        return -1;
    }

    struct tcp_pcb *pcb = tcp_new_pcb();
    if (!pcb) {
        return -1;
    }

    int fd = find_free_socket();
    if (fd < 0) {
        printf("t_socket: no free socket handles\n");
        free(pcb);
        return -1;
    }

    sockets[fd].pcb = pcb;
    printf("t_socket: created socket %d\n", fd);
    return fd;
}

int t_bind(int sockfd, const struct sockaddr_in *addr) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    if (pcb->state != TCP_CLOSED) {
        printf("t_bind: socket not in CLOSED state\n");
        return -1;
    }

    pcb->local_ip = addr->sin_addr;
    pcb->local_port = my_ntohs(addr->sin_port);
    printf("t_bind: socket %d bound to port %d\n", sockfd, pcb->local_port);
    return 0;
}

int t_listen(int sockfd, int backlog) {
    (void)backlog;
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    if (pcb->state != TCP_CLOSED) {
        printf("t_listen: socket not in CLOSED state\n");
        return -1;
    }
    pcb->state = TCP_LISTEN;
    printf("t_listen: socket %d now LISTENing on port %d\n", sockfd, pcb->local_port);
    return 0;
}

int t_accept(int sockfd, struct sockaddr_in *addr) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *listen_pcb = sockets[sockfd].pcb;
    if (listen_pcb->state != TCP_LISTEN) {
        printf("t_accept: socket not LISTENing\n");
        return -1;
    }

    printf("t_accept: waiting for connection...\n");
    while (1) {
        struct tcp_pcb *pcb = tcp_pcbs;
        while (pcb) {
            if (pcb->state == TCP_ESTABLISHED &&
                pcb->local_port == listen_pcb->local_port &&
                pcb != listen_pcb) {
                int new_fd = find_free_socket();
                if (new_fd < 0) {
                    return -1;
                }
                sockets[new_fd].pcb = pcb;
                if (addr) {
                    addr->sin_family = AF_INET;
                    addr->sin_addr = pcb->remote_ip;
                    addr->sin_port = my_htons(pcb->remote_port);
                }
                printf("t_accept: accepted connection on socket %d\n", new_fd);
                return new_fd;
            }
            pcb = pcb->next;
        }
        usleep(10000); /* 10ms */
    }
}

int t_connect(int sockfd, const struct sockaddr_in *addr) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    if (pcb->state != TCP_CLOSED) {
        printf("t_connect: socket not in CLOSED state\n");
        return -1;
    }

    pcb->remote_ip = addr->sin_addr;
    pcb->remote_port = my_ntohs(addr->sin_port);
    pcb->local_ip = my_htonl(0xC0A80A01u); /* 192.168.10.1 */

    static uint32_t isnc = 5000;
    pcb->iss = isnc++;
    pcb->snd_nxt = pcb->iss;
    pcb->snd_una = pcb->iss;

    pcb->state = TCP_SYN_SENT;
    tcp_send_raw(pcb, TCP_SYN, NULL, 0);
    pcb->snd_nxt++;
    printf("t_connect: SYN sent to 0x%08X:%d\n", pcb->remote_ip, pcb->remote_port);

    while (pcb->state != TCP_ESTABLISHED) {
        if (pcb->state == TCP_CLOSED) {
            printf("t_connect: connection failed\n");
            return -1;
        }
        usleep(10000); /* 10ms */
    }
    printf("t_connect: connection established!\n");
    return 0;
}

int t_send(int sockfd, const void *buf, size_t len) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    if (pcb->state != TCP_ESTABLISHED) {
        printf("t_send: not established\n");
        return -1;
    }
    int ret = tcp_send(pcb, buf, len);
    tcp_output(pcb);
    return ret;
}

int t_recv(int sockfd, void *buf, size_t len) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    return tcp_recv(pcb, buf, len);
}

int t_close(int sockfd) {
    if (sockfd < 1 || sockfd >= MAX_SOCKETS || !sockets[sockfd].pcb) {
        return -1;
    }
    struct tcp_pcb *pcb = sockets[sockfd].pcb;
    switch (pcb->state) {
        case TCP_ESTABLISHED:
            tcp_send_raw(pcb, TCP_FIN | TCP_ACK, NULL, 0);
            pcb->snd_nxt++;
            pcb->state = TCP_FIN_WAIT_1;
            printf("t_close: sent FIN, state=FIN_WAIT_1\n");
            break;
        case TCP_CLOSE_WAIT:
            tcp_send_raw(pcb, TCP_FIN | TCP_ACK, NULL, 0);
            pcb->snd_nxt++;
            pcb->state = TCP_LAST_ACK;
            printf("t_close: sent FIN, state=LAST_ACK\n");
            break;
        default:
            printf("t_close: closing in state %d\n", pcb->state);
            pcb->state = TCP_CLOSED;
            break;
    }
    sockets[sockfd].pcb = NULL;
    return 0;
}
