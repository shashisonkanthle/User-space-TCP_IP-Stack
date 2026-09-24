CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Iinclude -pthread
TARGET = my_stack

SRCS = src/main.c src/tun.c src/ip.c src/icmp.c src/udp.c src/tcp.c src/socket_api.c src/http_server.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
