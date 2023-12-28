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

int tun_alloc(char *dev)
{
    int fd, ret;
    struct ifreq ifr;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ret = ioctl(fd, TUNSETIFF, &ifr);
    if (ret < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }

    strncpy(dev, ifr.ifr_name, IFNAMSIZ);
    return fd;
}

int tun_up(char *dev)
{
    int fd, ret;
    struct ifreq ifr;

    fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_flags = IFF_UP | IFF_RUNNING;

    ret = ioctl(fd, SIOCSIFFLAGS, &ifr);
    if (ret < 0)
        perror("ioctl(SIOCSIFFLAGS)");

    close(fd);
    return ret;
}

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
        return -1;
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

int init_tun(char *dev, struct in6_addr *ia)
{
    int fd, ret;

    fd = tun_alloc(dev);
    if (fd < 0)
        return fd;

    ret = tun_up(dev);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    ret = tun_setroute(dev, ia);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    printf("Created dev %s\n", dev);
    return fd;
}

