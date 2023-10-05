#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_packet.h>

#ifndef IPV6_MEADCAST
#define IPV6_MEADCAST 253
#endif /* ifndef IPV6_MEADCAST */

struct mdc_hdr {
    struct ip6_rthdr rthdr;
    uint8_t num_dst;
    uint32_t
        discovery: 1,
        response:  1,
        hopcount:  6,
        reserved: 24;
    uint32_t dlvmap;
    uint32_t rtmap;
    struct in6_addr addr[];
} __attribute__((packed));

void get_src_mac(int *sockfd, char *if_name, uint8_t *src_mac);
void print_mac(uint8_t *mac, char *info);
void print_ip6_addr(char *str, const struct in6_addr *addr);
void set_sock_addr(struct sockaddr_ll *dev, uint8_t *dst_mac, char *if_name);

void set_iphdr(struct ip6_hdr *hdr, char *src, char *dst, uint8_t nh,
            uint16_t pl);
void set_hbh_exthdr(struct ip6_hbh *hdr, uint8_t nh);
void set_rt_hdr(struct mdc_hdr *hdr, uint8_t nh, uint8_t len, uint8_t type,
            uint8_t segleft, char **dsts, uint8_t num_dst);

void send_eth_frame(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *data, size_t data_len, uint16_t proto);
void send_empty_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst);
void send_hbh_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst);
void send_rt_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst, uint8_t num_dst);

int main(int argc, char *argv[])
{
    int sockfd;
    char *if_name = "eth0"; // change
    char *src_ip = "2a00:1450:4016:80a::200e"; // change

    char *dst_ip = "2a00:1450:4016:80a::200e"; // change
    // set to mac of default gw if dst_ip is in another network
    uint8_t dst_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // change

    uint8_t *src_mac, *eth_frame;
    struct sockaddr_ll dev;

    src_mac = malloc(ETH_ALEN);
    eth_frame = malloc(IP_MAXPACKET);

    get_src_mac(&sockfd, if_name, src_mac);
    print_mac(src_mac, if_name);
    print_mac(dst_mac, dst_ip);

    set_sock_addr(&dev, dst_mac, if_name);
    // send_empty_ip(&sd, &dev, sizeof(dev), ether_frame, src_ip, dst_ip);
    // send_hbh_ip(&sockfd, &dev, sizeof(dev), eth_frame, src_ip, dst_ip);
    send_rt_ip(&sockfd, &dev, sizeof(dev), eth_frame, src_ip, dst_ip, 3);

    // dealloc
    free(src_mac);
    free(eth_frame);

    return EXIT_SUCCESS;
}

void send_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst, uint8_t nh, size_t pl)
{
    size_t eth_len = sizeof(struct ip6_hdr) + pl;
    set_iphdr((struct ip6_hdr *)eth_frame, src, dst, nh, pl);
    send_eth_frame(sockfd, dev, dev_len, eth_frame, eth_len, ETH_P_IPV6);
}

void send_empty_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst)
{
    size_t eth_len, ip_pl;
    ip_pl = 0;
    eth_len = sizeof(struct ip6_hdr) + ip_pl;

    eth_frame = malloc(eth_len);
    send_ip(sockfd, dev, dev_len, eth_frame, src, dst, IPPROTO_NONE, ip_pl);
}

void send_hbh_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst)
{
    size_t eth_len, ip_pl;
    struct ip6_hbh *hbh_hdr;

    ip_pl = 8;
    eth_len = sizeof(struct ip6_hdr) + ip_pl;

    eth_frame = malloc(eth_len);
    hbh_hdr = (struct ip6_hbh *) (eth_frame + sizeof(struct ip6_hdr));

    set_hbh_exthdr(hbh_hdr, IPPROTO_NONE);
    send_ip(sockfd, dev, dev_len, eth_frame, src, dst, IPPROTO_HOPOPTS, ip_pl);
}

/* Creates a list of ip addresses */
char **create_dst_list(size_t len)
{
    // pool of meadcast endoints to pick from
    static char *src[2] = {
        "2a00:1450:4016:80a::200e", // change
        "2a00:1450:4016:80a::200e" // change
    };

    char **dsts = malloc(len * sizeof(char *));

    for (int i = 0; i < len; i++) {
        dsts[i] = malloc(INET6_ADDRSTRLEN * sizeof(char));
        strncpy(dsts[i], src[i%2], INET6_ADDRSTRLEN);
    }

    return dsts;
}

void send_rt_ip(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
            uint8_t *eth_frame, char *src, char *dst, uint8_t num_dst)
{
    size_t eth_len, ip_pl, md_pl, pad, rt_len;
    char **dsts;
    struct mdc_hdr *mdc_hdr;

    // fix size header plus var size (addresses)
    md_pl = sizeof(struct mdc_hdr) + num_dst * sizeof(struct in6_addr);

    // num of padding bytes to get a multiple of 8 bytes
    pad = (8 - (md_pl & 7)) & 7;
    ip_pl = md_pl + pad;

    // len field in rt hdr, doesn't consider first octet
    rt_len = ip_pl / 8 - 1;
    eth_len = sizeof(struct ip6_hdr) + ip_pl;

    eth_frame = malloc(eth_len);
    mdc_hdr = (struct mdc_hdr *) (eth_frame + sizeof(struct ip6_hdr));

    dsts = create_dst_list(num_dst);
    // for (int i = 0; i < num_dst; i++)
    //     printf("IP %d: %s\n", i, t[i]);

    printf("Creating MEADcast packet of size (total/pad/rt_len): %zu/%zu/%zu "
        "and %d receivern\n", ip_pl, pad, rt_len, num_dst);

    set_rt_hdr(mdc_hdr, IPPROTO_NONE, rt_len, IPV6_MEADCAST, 0, dsts, num_dst);
    send_ip(sockfd, dev, dev_len, eth_frame, src, dst, IPPROTO_ROUTING, ip_pl);

    // dealloc
    for (int i = 0; i < num_dst; i++)
        free(dsts[i]);
    free(dsts);
}

/* Gets MAC address by interface name */
void get_src_mac(int *sockfd, char *if_name, uint8_t *src_mac)
{
    struct ifreq ifreq;

    // Submit request for a socket descriptor to look up interface.
    if ((*sockfd = socket(PF_PACKET, SOCK_DGRAM, IPPROTO_RAW)) < 0) {
      perror("socket() failed to get socket descriptor for using ioctl()");
      exit(EXIT_FAILURE);
    }

    // Use ioctl() to look up interface name and get its MAC address.
    memset(&ifreq, 0, sizeof(ifreq));
    snprintf(ifreq.ifr_name, sizeof (ifreq.ifr_name), "%s", if_name);
    if (ioctl(*sockfd, SIOCGIFHWADDR, &ifreq) < 0) {
      perror("ioctl() failed to get source MAC address");
      exit(EXIT_FAILURE);
    }
    close(*sockfd);
  
    // Copy source MAC address.
    memcpy(src_mac, ifreq.ifr_hwaddr.sa_data, ETH_ALEN);
}

void print_mac(uint8_t *mac, char *info)
{
    // Report source MAC address to stdout.
    printf ("MAC address for %s is ", info);
    for (int i=0; i<5; i++)
      printf ("%02x:", mac[i]);
    printf ("%02x\n", mac[5]);
}

void set_sock_addr(struct sockaddr_ll *dev, uint8_t *dst_mac, char *if_name)
{
    // Find interface index from interface name and store index in
    // struct sockaddr_ll device, which will be used as an argument of sendto().
    memset(dev, 0, sizeof(struct sockaddr_ll));
    if ((dev->sll_ifindex = if_nametoindex (if_name)) == 0) {
      perror("if_nametoindex() failed to obtain interface index ");
      exit(EXIT_FAILURE);
    }
    // printf("Index for interface %s is %i\n", if_name, dev->sll_ifindex);

    dev->sll_family = AF_PACKET;
    dev->sll_protocol = htons(ETH_P_IPV6);
    dev->sll_halen = 6;

    memcpy(dev->sll_addr, dst_mac, ETH_ALEN);
}

/* Creates an ip header */
void set_iphdr(struct ip6_hdr *hdr, char *src, char *dst, uint8_t nh, uint16_t pl)
{
    int status;

    // Version (4 bits), Traffic class (8 bits), Flow label (20 bits)
    hdr->ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
    hdr->ip6_plen = htons(pl);
    hdr->ip6_nxt = nh;
    hdr->ip6_hops = 255;

    if ((status = inet_pton(AF_INET6, src, &hdr->ip6_src)) != 1) {
        fprintf(stderr,
                "inet_pton() failed for source address.\n"
                "Error message: %s",
                strerror(status));
        exit(EXIT_FAILURE);
    }

    if ((status = inet_pton(AF_INET6, dst, &hdr->ip6_dst)) != 1) {
        fprintf(stderr,
                "inet_pton() failed for destination address.\n"
                "Error message: %s",
                strerror(status));
        exit(EXIT_FAILURE);
    }
}

/* creates a minimal hop-by-hop header */
void set_hbh_exthdr(struct ip6_hbh *hdr, uint8_t nh)
{
    // create opt pointer after hbh hdr
    uint8_t *opt = (uint8_t *)((struct ip6_hbh *)hdr);

    hdr->ip6h_nxt = nh;
    hdr->ip6h_len = 0;

    // expands hbh to min size of 8 bytes
    opt[0] = 1; // type
    opt[1] = 4; // len
    opt[2] = 0; // padN
    opt[3] = 0; 
    opt[4] = 0; 
    opt[5] = 0; 
}

/* creates a MEADcast routing header */
void set_rt_hdr(struct mdc_hdr *hdr, uint8_t nh, uint8_t len, uint8_t type,
        uint8_t segleft, char **dsts, uint8_t num_dst)
{
    int status;
    char *p;

    hdr->rthdr.ip6r_nxt = nh;
    hdr->rthdr.ip6r_len = len;
    hdr->rthdr.ip6r_type = type;
    hdr->rthdr.ip6r_segleft = segleft;
    hdr->num_dst = num_dst;
    hdr->discovery = 1;
    hdr->response = 1;
    hdr->hopcount = 5;
    hdr->reserved = 0;
    hdr->dlvmap = htonl(16);
    hdr->rtmap = htonl(253);

    for (int i = 0; i < num_dst; i++) {
        if ((status = inet_pton(AF_INET6, dsts[i], &hdr->addr[i])) != 1) {
            fprintf(stderr,
                    "inet_pton() failed for destination address: %s.\n"
                    "Error message: %s",
                    dsts[i], strerror(status));
            exit(EXIT_FAILURE);
        }
    }
}

void send_eth_frame(int *sockfd, struct sockaddr_ll *dev, size_t dev_len,
        uint8_t *data, size_t data_len, uint16_t proto)
{
    int bytes;

    if ((*sockfd = socket(PF_PACKET, SOCK_DGRAM, htons(proto))) < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    if ((bytes = sendto(*sockfd, data, data_len, 0, (struct sockaddr *)dev, dev_len)) <= 0) {
        perror("sendto() failed");
        exit(EXIT_FAILURE);
    }

    close(*sockfd);
}

void print_ip6_addr(char *str, const struct in6_addr *addr) {
    sprintf(str,
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
            (int)addr->s6_addr[0], (int)addr->s6_addr[1],
            (int)addr->s6_addr[2], (int)addr->s6_addr[3],
            (int)addr->s6_addr[4], (int)addr->s6_addr[5],
            (int)addr->s6_addr[6], (int)addr->s6_addr[7],
            (int)addr->s6_addr[8], (int)addr->s6_addr[9],
            (int)addr->s6_addr[10], (int)addr->s6_addr[11],
            (int)addr->s6_addr[12], (int)addr->s6_addr[13],
            (int)addr->s6_addr[14], (int)addr->s6_addr[15]);
}
