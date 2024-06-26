#ifndef TUN_H
#define TUN_H

#include <netinet/in.h>

int init_tun(char *dev, struct in6_addr *ia, int mtu);
#endif // !TUN_H
