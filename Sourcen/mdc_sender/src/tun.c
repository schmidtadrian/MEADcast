#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
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

int init_tun(char *dev)
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

    printf("Created dev %s\n", dev);
    return fd;
}

