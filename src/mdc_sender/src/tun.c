#include "util.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/in6.h>
#include <linux/ipv6_route.h>
#include <linux/route.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* RFC 8200 requires a min MTU of 1280 bytes. */
#ifndef MIN_IPV6_MTU
#define MIN_IPV6_MTU 1280
#endif /* ifndef MIN_IPV6_MTU */

/* Creates TUN device of name `dev`.
 * If `dev` is NULL, the os sets a default name.
 * The name of the created interface gets written to `dev`. */
int tun_alloc(char *dev)
{
    int fd, ret;
    struct ifreq ifr;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ret = ioctl(fd, TUNSETIFF, &ifr);
    if (ret < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return ret;
    }

    strncpy(dev, ifr.ifr_name, IFNAMSIZ);
    return fd;
}

/* Sets mtu of interface with name `dev` to `mtu` bytes.
 * On success returns the interface's new mtu. On error returns -1. */
int tun_up(char *dev, int mtu)
{
    int fd, ret;
    struct ifreq ifr;

    fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_flags = IFF_UP | IFF_RUNNING;

    ret = ioctl(fd, SIOCSIFFLAGS, &ifr);
    if (ret < 0)
        perror("ioctl(SIOCSIFFLAGS)");

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_mtu = mtu < MIN_IPV6_MTU ? MIN_IPV6_MTU : mtu;

    ret = ioctl(fd, SIOCSIFMTU, &ifr);
    if (ret < 0) {
        perror("ioctl(SIOCGIFMTU)");
        goto error;
    }

    ret = ifr.ifr_ifru.ifru_mtu;

error:
    close(fd);
    return ret;
}

/* Set host route to `ia` via interface of name `dev`. */
int tun_setroute(char *dev, struct in6_addr *ia)
{
    struct in6_rtmsg rt;
    struct ifreq ifr;
    int fd, ret;

    memset(&rt, 0, sizeof(rt));
    memcpy(&rt.rtmsg_dst, ia, sizeof(struct in6_addr));

    rt.rtmsg_dst_len = 128;
    rt.rtmsg_flags = RTF_UP | RTF_HOST;
    rt.rtmsg_metric = 1;

    fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ret = ioctl(fd, SIOCGIFINDEX, &ifr);
    if (ret < 0) {
        perror("ioctl(SIOCGIFINDEX)");
        goto exit;
    }

    rt.rtmsg_ifindex = ifr.ifr_ifindex;
    ret = ioctl(fd, SIOCADDRT, &rt);
    if (ret < 0)
        perror("ioctl(SIOCADDRT)");

exit:
    close(fd);
    return ret;
}

int init_tun(char *dev, struct in6_addr *ia, int mtu)
{
    int fd, ret;

    fd = tun_alloc(dev);
    if (fd < 0)
        return fd;

    ret = tun_up(dev, mtu);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    printf("Created dev %s with MTU of %d\n", dev, ret);
    if (mtu < ret)
        printf("Still don't send packets bigger than %d "
               "(IPv6 requires a min MTU of %d)\n", mtu, MIN_IPV6_MTU);

    ret = tun_setroute(dev, ia);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    printf("Send your traffic to ");
    print_ia(ia);
    printf("\n");
    return fd;
}

