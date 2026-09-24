#include "socket_api.h"
#include "net_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Simple HTTP response
 * HTTP/1.1 200 OK\r\n
 * Content-Length: 13\r\n
 * Content-Type: text/html\r\n
 * \r\n
 * Hello, World!
 */
static const char *http_response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "Hello, World!";

int http_server_run(void) {
    /* Step 1: Create a listening socket */
    int listen_fd = t_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }

    /* Step 2: Bind to port 80 */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = 0x00000000; /* INADDR_ANY = 0.0.0.0 */
    addr.sin_port = my_htons(80);  /* Port 80 (network byte order) */

    if (t_bind(listen_fd, &addr) < 0) {
        fprintf(stderr, "Failed to bind to port 80\n");
        /* Try port 8080 if 80 fails */
        addr.sin_port = my_htons(8080);
        if (t_bind(listen_fd, &addr) < 0) {
            fprintf(stderr, "Failed to bind to port 8080\n");
            return 1;
        }
    }

    /* Step 3: Start listening */
    if (t_listen(listen_fd, 5) < 0) {
        fprintf(stderr, "Failed to listen\n");
        return 1;
    }

    printf("HTTP server listening on port %d\n", my_ntohs(addr.sin_port));
    printf("Open http://192.168.10.1 in your browser\n");

    /* Step 4: Accept and handle connections forever */
    while (1) {
        struct sockaddr_in client_addr;
        int conn_fd = t_accept(listen_fd, &client_addr);
        if (conn_fd < 0) {
            fprintf(stderr, "Accept failed\n");
            continue;
        }
        printf("New connection on socket %d\n", conn_fd);

        /* Step 5: Read the HTTP request */
        char req[1024];
        int n = 0;
        for (int retries = 0; retries < 100 && n <= 0; retries++) {
            n = t_recv(conn_fd, req, sizeof(req) - 1);
            if (n <= 0) {
                usleep(10000); /* wait 10ms for data */
            }
        }
        if (n > 0) {
            req[n] = '\0';
            printf("Received request:\n%s\n", req);
        }

        /* Step 6: Send HTTP response */
        int resp_len = strlen(http_response);
        t_send(conn_fd, http_response, resp_len);

        /* Step 7: Close the connection */
        t_close(conn_fd);
        printf("Connection closed\n\n");
    }
    return 0;
}
