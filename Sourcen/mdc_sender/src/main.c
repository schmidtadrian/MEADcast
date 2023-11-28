#include "core.h"
#include "group.h"
#include "list.h"
#include "test.h"
#include "tree.h"
#include "util.h"
#include <linux/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;
    char tif[IFNAMSIZ];
    char bif[IFNAMSIZ];
    char baddr[INET6_ADDRSTRLEN];
    char paddr[INET6_ADDRSTRLEN];
    uint16_t bport, pport;

    // test();

    if (argc < 6) {
        printf("Usage <tun> <bind_if> <bind_addr> <bind_port> <peer_addr> <peer_port>\n");
        return EXIT_FAILURE;
    }

    strncpy(tif, argv[1], sizeof(tif));
    strncpy(bif, argv[2], sizeof(bif));
    strncpy(baddr, argv[3], sizeof(baddr));
    strncpy(paddr, argv[5], sizeof(paddr));

    bport = atoi(argv[4]);
    if (bport < 1) {
        printf("Invalid port\n");
        return EXIT_FAILURE;
    }

    pport = atoi(argv[6]);
    if (pport < 1) {
        printf("Invalid port\n");
        return EXIT_FAILURE;
    }
    
    ret = init_group(baddr, 0);
    if (ret < 0)
        return EXIT_FAILURE;

    ret = join(paddr, pport);
    if (ret < 0)
        return EXIT_FAILURE;

    // ret = join("fd14:0:6::f", 9999);
    // if (ret < 0)
    //     return EXIT_FAILURE;

    ret = join("fd14:0:8::1", 9999);
    if (ret < 0)
        return EXIT_FAILURE;

    ret = start(tif, bif, baddr, bport, 1600);
    if (ret < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
