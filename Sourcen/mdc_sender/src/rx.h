#ifndef RX_H
#define RX_H

#include "group.h"
#include <stddef.h>
#include <stdint.h>

struct rx_targs {
    int tun_fd;
    int ip6_fd;
    int udp_fd;
    int tcp_fd;
    struct addr *ta;
};

void init_rx(size_t n);
int rx_disc(int fd);

static uint8_t *rxbuf = NULL;
static size_t rxlen = 0;

#endif // !RX_H
