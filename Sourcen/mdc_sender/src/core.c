#include "core.h"
#include "discover.h"
#include "group.h"
#include "rx.h"
#include "tree.h"
#include "tun.h"
#include "tx.h"
#include "util.h"
#include <bits/pthreadtypes.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

size_t get_mdc_pkt_size(size_t num_dst)
{
    size_t ip_pl, mdc_pl, pad;

    mdc_pl = sizeof(struct ip6_mdc_hdr) + num_dst * sizeof(struct in6_addr);

    // num of padding bytes to get a multiple of 8 bytes
    pad = (8 - (mdc_pl & 7)) & 7;

    return mdc_pl + pad;
}

int init_fds(int *tun_fd, int *mdc_fd, int *ip6_fd,
             char *tif, char *bif, struct addr *addr)
{
    int ret;
    const int on = 1;
    struct sockaddr_in6 l4sa;
    struct ifreq ifr;

    // TODO set TUN MTU according to bind interface MTU & group size
    *tun_fd = init_tun(tif);
    if (*tun_fd < 1)
        return *tun_fd;

    *mdc_fd = socket(AF_INET6, SOCK_RAW, IPPROTO_ROUTING);
    if (*mdc_fd < 0) {
        perror("socket (mdc)");
        ret = *mdc_fd;
        goto close_tun;
    }

    *ip6_fd = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (*ip6_fd < 0) {
        perror("socket (ip6)");
        ret = *ip6_fd;
        goto close_mdc;
    } 

    strncpy(ifr.ifr_name, bif, IFNAMSIZ);
    ret = setsockopt(*ip6_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
    if (ret < 0) {
        perror("setsockopt (BINDTODEV)");
        goto close_ip6;
    }

    ret = setsockopt(*mdc_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
    if (ret < 0) {
        perror("setsockopt (BINDTODEV)");
        goto close_ip6;
    }

    ret = bind(*mdc_fd, (struct sockaddr *) &addr->sa, sizeof(addr->sa));
    if (ret < 0) {
        perror("bind (mdc)");
        goto close_ip6;
    }
    printf("MEADcast bound to ");
    print_ia(&addr->sa.sin6_addr);
    printf("/%d\n", addr->port);

    ret = bind(*ip6_fd, (struct sockaddr *) &addr->sa, sizeof(addr->sa));
    if (ret < 0) {
        perror("bind (ip6)");
        goto close_ip6;
    }

    printf("Unicast bound to ");
    print_ia(&addr->sa.sin6_addr);
    printf("/%d\n", addr->port);

    return 0;    

close_ip6:
    close(*ip6_fd);
close_mdc:
    close(*mdc_fd);
close_tun:
    close(*tun_fd);

    return ret;
}

int start(char *tif, char *bif, char *baddr, uint16_t bport, size_t mtu)
{
    int ret;
    int tun_fd, mdc_fd, ip6_fd;
    struct addr ba;
    struct dx_targs *dxargs;
    pthread_t txid, rxid, dxid;

    ret = init_sa(&ba.sa, baddr, 0);
    if (ret < 0)
        return -1;
    ba.port = bport;

    ret = init_fds(&tun_fd, &mdc_fd, &ip6_fd, tif, bif, &ba);
    if (ret) {
        printf("ERR INIT FDS\n");
        return -1;
    }

    init_rx(mtu - get_mdc_pkt_size(MAX_NUM_ADDR));

    txid = start_tx(tun_fd, mdc_fd, ip6_fd, mtu);
    if (txid < 0)
        return -1;

    // rxid = start_rx(tun_fd, ip6_fd, mtu);

    dxargs = malloc(sizeof(*dxargs));
    dxargs->fd = mdc_fd;
    dxargs->tout = 2;
    dxargs->tint = 5;
    dxargs->evlen = 10;
    ret = pthread_create(&dxid, NULL, (void *) start_dx, dxargs);
    if (ret) {
        perror("pthread_create (dx)");
        return -1;
    }

    pthread_join(txid, NULL);
    pthread_join(dxid, NULL);
    return 0;
}
