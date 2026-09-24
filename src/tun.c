#include "tun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
/*struct ifreq {
    char ifr_name[IFNAMSIZ]; Interface name, e.g., "eth0" or "wlan0" 
    union {
        struct sockaddr ifr_addr;      IP address 
        struct sockaddr ifr_dstaddr;   Point-to-point destination address 
        struct sockaddr ifr_broadaddr;  Broadcast address 
        struct sockaddr ifr_netmask;   Subnet mask 
        struct sockaddr ifr_hwaddr;     MAC / Hardware address 
        short           ifr_flags;     Interface operational flags (e.g., IFF_UP) 
        int             ifr_ifindex;   Interface index number 
        int             ifr_metric;     Routing metric 
        int             ifr_mtu;       Maximum Transmission Unit (MTU) 
        struct ifmap    ifr_map;        Hardware parameters mapping 
        char            ifr_slave[IFNAMSIZ];  Driver slave interface name 
        char            ifr_newname[IFNAMSIZ]; Target name for renaming 
        char           *ifr_data;       Driver-specific private data pointer 
    };
};*/

int tun_create(char *dev, const char *ip, int prefix) {
    struct ifreq ifr;// general interface request structure used for all network device configuration
    int fd, err;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("tun: open /dev/net/tun failed");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;// IFF_TUN: TUN device, IFF_NO_PI: no packet information

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
    }

    err = ioctl(fd, TUNSETIFF, (void *)&ifr);// talks to device driver via file descriptor
    if (err < 0) {
        perror("tun: ioctl TUNSETIFF failed");
        close(fd);
        return -1;
    }

    strcpy(dev, ifr.ifr_name);
    /*Create a socket for configuring the TUN device because 
    To talk to the kernel's networking subsystem, you must use a socket descriptor.*/
    int sock = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sock < 0) {
        perror("tun: socket failed");
        close(fd);
        return -1;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr->sin_addr);
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
        perror("tun: ioctl SIOCSIFADDR failed");
    }

    struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
    netmask->sin_family = AF_INET;
    netmask->sin_addr.s_addr = htonl(0xFFFFFFFF << (32 - prefix));
    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
        perror("tun: ioctl SIOCSIFNETMASK failed");
    }

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("tun: ioctl SIOCGIFFLAGS failed");
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("tun: ioctl SIOCSIFFLAGS failed");
    }

    ifr.ifr_mtu = 1500;
    if (ioctl(sock, SIOCSIFMTU, &ifr) < 0) {
        perror("tun: ioctl SIOCSIFMTU failed");
    }

    close(sock);
    return fd;
}

int tun_read(int fd, uint8_t *buf, int len) {
    return read(fd, buf, len);// blocking read from the TUN device file descriptor, reading up to len bytes into buf
}

int tun_write(int fd, const uint8_t *buf, int len) {
    return write(fd, buf, len);// blocking write to the TUN device file descriptor, writing len bytes from buf
}
