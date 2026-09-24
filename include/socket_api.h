#ifndef SOCKET_API_H
#define SOCKET_API_H

#include <stddef.h>
#include <stdint.h>

/* === CUSTOM SOCKET API === */
/* These functions mimic the standard BSD socket API
 * but operate on YOUR TCP/IP stack instead of the kernel.
 *
 * t_socket: create a new socket (returns a handle, like a file descriptor)
 * t_bind: assign a local IP and port to a socket
 * t_listen: mark a socket as passive (ready to accept connections)
 * t_accept: accept an incoming connection (blocking)
 * t_connect: initiate an active connection to a remote host
 * t_send: send data on an established connection
 * t_recv: receive data from an established connection
 * t_close: close a connection (sends FIN)
 */

#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6

/* sockaddr_in: IPv4 address structure */
struct sockaddr_in {
    uint16_t sin_family; /* AF_INET */
    uint16_t sin_port;   /* Port number (network byte order) */
    uint32_t sin_addr;   /* IP address (network byte order) */
};

int t_socket(int domain, int type, int protocol);
int t_bind(int sockfd, const struct sockaddr_in *addr);
int t_listen(int sockfd, int backlog);
int t_accept(int sockfd, struct sockaddr_in *addr);
int t_connect(int sockfd, const struct sockaddr_in *addr);
int t_send(int sockfd, const void *buf, size_t len);
int t_recv(int sockfd, void *buf, size_t len);
int t_close(int sockfd);

#endif
