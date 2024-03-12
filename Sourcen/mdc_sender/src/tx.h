#ifndef TX_H
#define TX_H

#include <linux/if_tun.h>
#include <linux/ipv6.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>


struct tx_targs {
    int tun_fd;
    int mdc_fd;
    int ip6_fd;
};

void init_tx(size_t mtu);
void tx_loop(struct tx_targs *args);
pthread_t start_tx(int tun_fd, int mdc_fd, int ip6_fd, size_t mtu);
int send_dcvr(int fd);
void set_data_hdr(void **buf, struct in6_addr *addrs, size_t len, uint32_t bm);

#endif // !TX_H
