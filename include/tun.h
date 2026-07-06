#ifndef TUN_H
#define TUN_H
#include <stdint.h>

int tun_create(char *dev, const char *ip, int prefix);

int tun_read(int fd, uint8_t *buf, int len);

int tun_write(int fd, const uint8_t *buf, int len);

#endif