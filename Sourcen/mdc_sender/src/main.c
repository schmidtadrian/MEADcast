#include "argp.h"
#include "core.h"
#include "group.h"
#include "list.h"
#include "test.h"
#include "tree.h"
#include "util.h"
#include <argp.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;

    // test(); // exit success
    init_args(&args, argc, argv);

    ret = init_group(&args.baddr, args.bport);
    if (ret < 0)
        return EXIT_FAILURE;

    for (struct paddr *i = args.paddr; i; i = i->n) {
        ret = join(&i->v, args.pport);
        if (ret < 0)
            return EXIT_FAILURE;
    }

    ret = start(args.tifname, args.bifname, &args.taddr, &args.baddr,
                args.bport, 1600, &args.dcvr_int, &args.dcvr_tout);
    if (ret < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
