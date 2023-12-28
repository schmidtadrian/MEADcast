#include "checksum.h"
#include "core.h"
#include "group.h"
#include "tree.h"
#include "tx.h"
#include "util.h"
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void set_rt_hdr(struct ip6_rthdr *hdr, uint8_t nh, uint8_t len, uint8_t type,
                uint8_t segleft)
{
    hdr->ip6r_nxt = nh;
    hdr->ip6r_len = len;
    hdr->ip6r_type = type;
    hdr->ip6r_segleft = segleft;
}

uint8_t set_disc_hdr(struct ip6_mdc_hdr *hdr)
{
    uint8_t rt_len;
    size_t mdc_len;

    hdr->dsts = 1;
    hdr->dcv  = 1;
    hdr->rsp  = 0;
    hdr->hops = 0;
    hdr->res  = 0;
    hdr->dlvm = 0;
    hdr->rtm  = 0;

    // rthdr len equals number of octets minus first octet.
    mdc_len = get_mdc_pkt_size(1);
    rt_len = mdc_len / 8 - 1;
    set_rt_hdr((struct ip6_rthdr *) &hdr->rthdr, IPPROTO_NONE, rt_len,
               IPV6_MEADCAST, 0);

    return mdc_len;
}

void set_data_hdr(void **buf, struct in6_addr *addrs, size_t len, uint32_t bm)
{
    uint8_t rt_len;
    size_t mdc_len;
    struct ip6_mdc_hdr *hdr;

    mdc_len = get_mdc_pkt_size(len);
    rt_len = mdc_len / 8 - 1;

    *buf = calloc(mdc_len, 1);
    hdr = *buf;

    set_rt_hdr(*buf, IPPROTO_UDP, rt_len, IPV6_MEADCAST, 0);

    hdr->dsts = len;
    hdr->dlvm = bm;
    hdr->rtm = bm;

    memcpy(hdr->addr, addrs, len * sizeof(*hdr->addr));
}

int send_disc(int fd)
{
    int ret;
    struct rcvr *i;
    struct sockaddr_in6 *dst;
    struct ip6_mdc_hdr *hdr;
    size_t len;

    hdr = (struct ip6_mdc_hdr *) txbuf;
    len = set_disc_hdr(hdr);

    for(i = get_rcvr(); i; i = i->next) {
        dst = &i->addr.sa;
        memcpy(&hdr->addr[0], &dst->sin6_addr, sizeof(*hdr->addr));

        printf("Sending DRQ to ");
        print_ia(&dst->sin6_addr);
        printf("\n");

        ret = sendto(fd, hdr, len, 0, (struct sockaddr *) dst, sizeof(*dst));
        if (ret < 1) {
            perror("sendto (discover)");
            return ret;
        }
    }

    return 0;
}

void init_tx(size_t mtu)
{
    size_t hlen;

    txlen = mtu;
    txbuf = malloc(txlen);

    hlen = get_mdc_pkt_size(MAX_NUM_ADDR);
    buflen = mtu - hlen;
    buf = txbuf + hlen;

    printf("tx/buf  len: %zu/%zu\n", txlen, buflen);
    printf("tx/buf addr: %p/%p\n", txbuf, buf);

    pi = (struct tun_pi *) buf;
    ip = (struct ipv6hdr *) (pi + 1);
}

/* Sends `l3pl` of size `plen` to each MEADcast header in `grp`.
 * Copies MEADcast header in front of `l3pl`.
 * This overwrites existing l3 headers!
 * Caller must ensure sufficient preceding space. */
int tx_mdc(int fd, struct tx_group *grp, uint8_t *l3pl, size_t plen)
{
    int n;
    size_t hlen;
    struct child *i;
    struct ip6_mdc_hdr *hdr, *mdc;
    static struct sockaddr_in6 dst = { AF_INET6, 0 };

    if (fd < 1 || !grp || !l3pl)
        return -1;

    for (i = grp->mdc; i; i = i->n) {

        hdr = i->v;
        if (hdr->dsts < 2)
            continue;

        dst.sin6_addr = hdr->addr[1];

        hlen = get_mdc_pkt_size(hdr->dsts);
        mdc = (struct ip6_mdc_hdr *) (l3pl - hlen);
        memcpy(mdc, hdr, hlen);

        n = sendto(fd, mdc, hlen + plen, 0,
                   (struct sockaddr *) &dst, sizeof(dst));

        if (n < 1) {
            perror("sendto (mdc)");
            return n;
        }
        printf("MDC: send %d bytes to ", n);
        print_mdc_hdr(hdr);

    }
    return 0;
}

int tx_uni(int fd, struct tx_group *grp, struct ipv6hdr *ip, size_t len)
{
    int n;
    size_t i;
    static struct sockaddr_in6 dst = { AF_INET6, 0 };
    static struct udphdr *udp;
    static struct tcphdr *tcp;
    struct addr *addr;

    if (fd < 1 || !grp || !ip)
        return -1;
    
    // printf("ip mem addr: %p\n", ip);
    for (addr = grp->uni, i = 0; addr && i < grp->nuni; addr++, i++) {

        dst.sin6_addr = addr->sa.sin6_addr;
        ip->daddr = addr->sa.sin6_addr;

        /* Best effort l4 header correction */
        if (ip->nexthdr == IPPROTO_UDP) {
            udp = (struct udphdr *) (ip + 1);
            printf("udp mem addr: %p\n", udp);
            udp->dest = htons(addr->port);
            udp->check = 0;
            udp->check = htons(
                net_checksum_tcpudp(ntohs(udp->len), 0, IPPROTO_UDP,
                                    &ip->saddr, &ip->daddr, udp));
        }

        /* if (ip->nexthdr == IPPROTO_TCP) {
            tcp = (struct tcphdr *) (ip + 1);
            tcp->dest = htons(addr->port);
            tcp->check = htons(1);
            tcp->check = htons(
                net_checksum_tcpudp(ntohs(ip->payload_len), 0, IPPROTO_TCP,
                                    &ip->saddr, &ip->saddr, tcp));
        } */

        n = sendto(fd, ip, len, 0, (struct sockaddr *) &dst, sizeof(dst));
        if (n < 1) {
            perror("sendto (uni)");
            return n;
        }
        printf("UNI: send %d bytes to ", n);
        print_ia(&dst.sin6_addr);
        printf("/%d\n", addr->port);
    }

    return 0;
}

void tx_loop(struct tx_targs *args)
{
    int n;
    size_t iplen;
    struct tx_group *grp;
    struct sockaddr_in6 dst;
    size_t pilen = sizeof(struct tun_pi);

    if (args->tun_fd < 1 || args->ip6_fd < 1)
        return;

    dst.sin6_family = AF_INET6;
    dst.sin6_port = 0;

    for (;;) {

        // TODO leave space at beginning for mdc hdr
        n = read(args->tun_fd, buf, buflen);
        if (n < 1) {
            perror("read");
            return;
        }
        // printf("TUN: read %d bytes\n", n);

        if (ntohs(pi->proto) != ETH_P_IPV6)
            return;

        grp = get_txg();
        if (!grp)
            return;

        // printf("L3 addr %p\n", ip);
        // printf("L4 addr %p\n", (ip + 1));
        iplen = n - sizeof(*pi);
        /* Unicast needs to be delivered first,
         * hence `tx_mdc` overwrites ip header. */
        tx_uni(args->ip6_fd, grp, ip, iplen);
        tx_mdc(args->mdc_fd, grp, (void *) (ip + 1), iplen - sizeof(*ip));
    }
}

pthread_t start_tx(int tun_fd, int mdc_fd, int ip6_fd, size_t mtu)
{
    int ret;
    pthread_t tid;
    struct tx_targs *args;

    if (init_txg())
        return -1;
    print_txg(get_txg());

    args = malloc(sizeof(*args));
    args->tun_fd = tun_fd;
    args->mdc_fd = mdc_fd;
    args->ip6_fd = ip6_fd;

    init_tx(mtu);
    ret = pthread_create(&tid, NULL, (void *) tx_loop, (void *) args);

    if (ret) {
        perror("pthread_create (tx)");
        return -1;
    }

    printf("Started tx thread\n");
    return tid;
}
