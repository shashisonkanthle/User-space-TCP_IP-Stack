# User-Space TCP/IP Stack in C

A complete, beginner-friendly, RFC-compliant user-space TCP/IP network stack implemented in C from scratch using Linux `TUN/TAP` virtual network interfaces.

---

## Architecture Overview

This project implements a layered network stack in user space, directly processing Layer 3 (IP) packets without relying on the Linux kernel's transport networking stack.

```text
+-------------------------------------------------------------+
|                  Application Layer                          |
|         Embedded HTTP Server (http_server.c)                |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|               Socket API Abstraction Layer                  |
|  t_socket(), t_bind(), t_listen(), t_accept(), t_send()...  |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                     Transport Layer                         |
|   TCP (tcp.c)        |    UDP (udp.c)    |   ICMP (icmp.c)  |
| - 3-Way Handshake    | - Pseudo-header   | - Echo Request   |
| - State Machine      |   Checksum        | - Echo Reply     |
| - Sliding Window     | - Datagram Dump   |                  |
| - RTO & Congestion   |                   |                  |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                     Network Layer                           |
|                      IPv4 (ip.c)                            |
| - Header Parsing & Validation (IHL, Version, Length)        |
| - One's Complement Internet Checksum (RFC 1071)             |
| - Protocol Demultiplexing (ICMP=1, TCP=6, UDP=17)           |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                   Interface Layer (tun.c)                   |
| - Linux TUN Device (/dev/net/tun)                           |
| - Host IP: 192.168.10.2 / Stack Endpoint: 192.168.10.1      |
+-------------------------------------------------------------+
```

---

## Features

- **Virtual Network Interface (`tun.c`)**: Creates and configures a Linux `tun0` interface with IP address `192.168.10.2/24` and routes packets to the stack endpoint at `192.168.10.1`.
- **IPv4 Protocol Layer (`ip.c`, `ip.h`)**:
  - Validates version, IHL, packet boundaries, and 16-bit one's complement header checksums.
  - Generates outgoing IPv4 packets with DF (Do Not Fragment) flag set.
  - Demultiplexes payload to ICMP, UDP, and TCP handlers.
- **ICMP Diagnostics (`icmp.c`, `icmp.h`)**:
  - Handles incoming ICMP Echo Requests (`ping`).
  - Swaps IP addresses and generates valid ICMP Echo Replies with recalculated checksums.
- **UDP Transport Layer (`udp.c`, `udp.h`)**:
  - Parses UDP datagrams and validates lengths.
  - Verifies 12-byte IPv4 pseudo-header checksums (RFC 768).
  - Logs payload content.
- **TCP State Machine & Transmission Control (`tcp.c`, `tcp.h`)**:
  - **Connection Lifecycle**: Full support for `CLOSED`, `LISTEN`, `SYN_SENT`, `SYN_RECEIVED`, `ESTABLISHED`, `FIN_WAIT_1`, `CLOSE_WAIT`, `LAST_ACK`, and `TIME_WAIT`.
  - **3-Way Handshake**: Responds to incoming client `SYN` with `SYN-ACK` and transitions to `ESTABLISHED` upon `ACK`.
  - **Sliding Window & Buffering**: 64 KB send/receive buffers for in-order delivery and flow control.
  - **Retransmission Timeout (RTO)**: Jacobson/Karels algorithm computing smoothed RTT (`srtt`) and RTT variance (`rttvar`) with exponential backoff per RFC 6298.
  - **Congestion Control**: Implements Tahoe congestion control with Slow Start (exponential `cwnd` growth) and Congestion Avoidance (linear growth).
- **Socket API Abstraction (`socket_api.c`, `socket_api.h`)**:
  - Provides a BSD-like interface (`t_socket`, `t_bind`, `t_listen`, `t_accept`, `t_connect`, `t_send`, `t_recv`, `t_close`).
  - Decouples user-space applications from internal TCP Protocol Control Blocks (`tcp_pcb`).
- **HTTP Server Application (`http_server.c`)**:
  - Listens on port 80 (fallback 8080).
  - Receives HTTP GET requests and delivers `HTTP/1.1 200 OK` responses with `Hello, World!`.

---

## File Structure

```text
.
├── Makefile              # Build automation script
├── README.md             # Project documentation
├── include/
│   ├── icmp.h            # ICMP header definition & function prototypes
│   ├── ip.h              # IPv4 header structure & IP declarations
│   ├── net_utils.h       # Byte-order helpers (endianness) & Internet checksum
│   ├── socket_api.h      # Custom socket API declarations & sockaddr_in
│   ├── tcp.h             # TCP header, state machine enum, PCB, & prototypes
│   ├── tun.h             # TUN device interface declarations
│   └── udp.h             # UDP header, pseudo-header, & prototypes
└── src/
    ├── http_server.c     # End-to-end HTTP web server demo
    ├── icmp.c            # ICMP echo processing
    ├── ip.c              # IP input validation & output packet generation
    ├── main.c            # Multithreaded entry point & event loop
    ├── socket_api.c      # Socket API implementation & handle mapping
    ├── tcp.c             # TCP state machine, sliding window, & timers
    ├── tun.c             # TUN device allocation & configuration ioctls
    └── udp.c             # UDP input handling & checksum verification
```

---

## RFC References

- **RFC 791**: Internet Protocol (IP)
- **RFC 792**: Internet Control Message Protocol (ICMP)
- **RFC 768**: User Datagram Protocol (UDP)
- **RFC 793**: Transmission Control Protocol (TCP)
- **RFC 1071**: Computing the Internet Checksum
- **RFC 1122**: Requirements for Internet Hosts -- Communication Layers
- **RFC 5681**: TCP Congestion Control
- **RFC 6298**: Computing TCP's Retransmission Timer

---

## Building and Running

### Prerequisites

- Linux operating system (kernel with TUN/TAP support enabled)
- `gcc` compiler and `make`
- `pthread` library support
- Root / `sudo` privileges or `CAP_NET_ADMIN` capability (required to create virtual TUN devices)

### Compile

```bash
make
```

To clean previous build artifacts:

```bash
make clean
```

### Run the Stack

```bash
sudo ./my_stack
```

The stack will configure `tun0` and output:

```text
Stack running on tun0 (fd=3)
Host side IP on tun0: 192.168.10.2
Stack endpoint IP: 192.168.10.1
Try: ping 192.168.10.1 or curl http://192.168.10.1
HTTP server listening on port 80
Open http://192.168.10.1 in your browser
t_accept: waiting for connection...
```

---

## Testing the Stack

Open a separate terminal window to test each layer:

### 1. Test ICMP Echo (Ping)
```bash
ping -c 4 192.168.10.1
```

### 2. Test HTTP Server
```bash
curl -v http://192.168.10.1
```
Or navigate to `http://192.168.10.1` in your browser.

### 3. Test Raw TCP with Netcat
```bash
nc 192.168.10.1 80
```

### 4. Inspect Packet Traffic
```bash
sudo tcpdump -i tun0 -nn -X
```

---

## License

MIT License. Educational implementation for understanding network protocol internals.
