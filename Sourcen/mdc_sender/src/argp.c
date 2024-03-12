#include <argp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argp.h"


struct arguments args;

static char doc[] = "Exemplary MEADcast sender";
static char args_doc[] = "[PEER ADDRESSES] [PEER PORT]";

static struct argp_option options[] = {
    { 0, 0, 0, 0, "Network:", OPT_GRP_NET},
    { "tun",          OPT_TIFNAME,     "ifname",   0, "Specify name of the created TUN device.", OPT_GRP_NET },
    { "interface",    OPT_BIFNAME,     "ifname",   0, "Specify interface to use.", OPT_GRP_NET },
    { "tun-address",  OPT_TADDR,       "ipv6",     0, "Specify host route to send traffic to.", OPT_GRP_NET },
    { "address",      OPT_BADDR,       "ipv6",     0, "Specify source address.", OPT_GRP_NET },
    { "port",         OPT_BPORT,       "port",     0, "Specify source port.", OPT_GRP_NET },

    { 0, 0, 0, 0, "Discovery:", OPT_GRP_DCVR },
    { "interval",     OPT_DCVR_INT,    "secs",     0, "Specify discovery interval.", OPT_GRP_DCVR },
    { "timeout",      OPT_DCVR_TOUT,   "secs",     0, "Specify discovery timeout.", OPT_GRP_DCVR },
    { "delay",        OPT_DCVR_DELAY,  "secs",     0, "Specify delay until initial discovery phase.", OPT_GRP_DCVR },

    { 0, 0, 0, 0, "Grouping:", OPT_GRP_GRP},
    { "max",          OPT_MAX_ADDRS,   "max",      0, "Specify the maximal number of addresses per MEADcast packet.", OPT_GRP_GRP },
    { "ok",           OPT_OK_ADDRS,    "ok",       0, "Specify whether a packet can be finished prematurely if it contains "
                                                      "an equal or greater number of addresses than `ok`. "
                                                      "Assigning `ok` a value greater or equal than `max` disables this feature.", OPT_GRP_GRP },
    { "min-leafs",    OPT_MIN_LEAFS,   "leafs",    0, "Routers with less leafs than `leafs` and less routers than `routers` get removed from tree.", OPT_GRP_GRP },
    { "min-routers",  OPT_MIN_ROUTERS, "routers",  0, "See `min-leafs`.", OPT_GRP_GRP },
    { "split",        OPT_SPLIT_NODES, 0,          0, "Specify whether leafs of same parent can be split into multiple packets.", OPT_GRP_GRP },
    { "merge",        OPT_MERGE_RANGE, "distance", 0, "Specify whether to merge leafs with distinct parent routers under an common ancestor. "
                                                      "`distance` determines the range within which leafs will be merged. "
                                                      "Assigning `distance` a value of 0 disables this feature.", OPT_GRP_GRP },

    { 0, 0, 0, 0, "Verbosity", 0},
    { "print-tree",   OPT_PRINT_TREE,   0,          0, "Prints topology tree to stdout", OPT_GRP_VERBOSE },
    { "print-groups", OPT_PRINT_TXG,    0,          0, "Prints grouping to stdout", OPT_GRP_VERBOSE },
    { 0 }
};

const char *help_info = "See --help for more information.";
const char *argp_program_version = "MEADcast sender v1";


void _parse_if(char **dst, char *arg, struct argp_state *state, char *info)
{
    if (strlen(arg) > IFNAMSIZ) {
        argp_failure(state, 1, 0, "Max %s ifname length is %d. %s",
                     info, IFNAMSIZ, help_info);
        exit(ARGP_KEY_ERROR);
    }
    *dst = arg;
}

void _parse_addr(struct in6_addr *addr, char *arg, struct argp_state *state,
                char *info)
{
    int ret;

    ret = inet_pton(AF_INET6, arg, addr);
    if (ret < 0) {
        argp_failure(state, 1, 0, "Invalid %s address family. %s",
                     info, help_info);
        exit(ARGP_KEY_ERROR);
    }

    if (ret < 1) {
        argp_failure(state, 1, 0, "Invalid %s address. %s",
                     info, help_info);
        exit(ARGP_KEY_ERROR);
    }
}

void _parse_port(uint16_t *port, char *arg, struct argp_state *state,
                char *info)
{
    if ((*port = atoi(arg)) > 0)
        return;

    argp_failure(state, 1, 0, "Invalid %s port: %s. %s", info, arg, help_info);
    exit(ARGP_KEY_ERROR);
}

void _parse_int(int *v, char *arg, struct argp_state *state, char *info,
                int min)
{
    if ((*v = atoi(arg)) >= min)
        return;

    argp_failure(state, 1, 0,
                 "Invalid value for %s: %s. %s must be at least %d. %s",
                 info, arg, arg, min, help_info);
    exit(ARGP_KEY_ERROR);
}

void _parse_peer(struct paddr **head, char *arg, struct argp_state *state)
{
    int ret;
    struct paddr *new, *i;
    new = malloc(sizeof(*new));

    ret = inet_pton(AF_INET6, arg, &new->v);
    if (!ret) {
        argp_failure(state, 1, 0, "Invalid peer address at index %d: %s. %s",
                     state->arg_num, arg, help_info);
        exit(ARGP_KEY_ERROR);
    }

    if (!*head) {
        new->n = NULL;
        *head = new;
        return;
    }

    for (i = *head; i->n; i = i->n) {}
    i->n = new;
}

void null_check(void *arg, struct argp_state *state, char *info)
{
    if (arg)
        return;

    argp_failure(state, 1, 0, "%s is required. %s", info, help_info);
    exit(ARGP_KEY_ERROR);
}

void empty_check(char *arg, struct argp_state *state, char *info)
{
    if (arg && strlen(arg) <= IFNAMSIZ)
        return;

    argp_failure(state, 1, 0, "%s is required. %s", info, help_info);
    exit(ARGP_KEY_ERROR);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {

        // network
        case OPT_TIFNAME:
            _parse_if((void *) &arguments->tifname, arg, state, "TUN");
            break;
        case OPT_BIFNAME:
            _parse_if((void *) &arguments->bifname, arg, state, "bind");
            break;
        case OPT_TADDR:
            _parse_addr(&arguments->taddr, arg, state, "tun");
            break;
        case OPT_BADDR:
            _parse_addr(&arguments->baddr, arg, state, "bind");
            break;
        case OPT_BPORT:
            _parse_port(&arguments->bport, arg, state, "bind");
            break;

        // discovery
        case OPT_DCVR_INT:
            arguments->dcvr_int.it_interval.tv_sec = atoi(arg);
            break;
        case OPT_DCVR_TOUT:
            arguments->dcvr_tout.it_interval.tv_sec = atoi(arg);
            break;
        case OPT_DCVR_DELAY:
            arguments->dcvr_int.it_value.tv_sec = atoi(arg);
            break;

        // grouping
        case OPT_MAX_ADDRS:
            _parse_int((int *)&arguments->max_addrs, arg, state, "MAX_ADDRS", 2);
            break;
        case OPT_OK_ADDRS:
            _parse_int((int *)&arguments->ok_addrs, arg, state, "OK_ADDRS", 2);
            break;
        case OPT_MIN_LEAFS:
            _parse_int((int *)&arguments->min_leafs, arg, state, "MIN_LEAFS", 1);
            break;
        case OPT_MIN_ROUTERS:
            _parse_int((int *)&arguments->min_routers, arg, state, "MIN_ROUTERS", 1);
            break;
        case OPT_SPLIT_NODES:
            arguments->split_nodes = true;
            break;
        case OPT_MERGE_RANGE:
            _parse_int((int *)&arguments->merge_range, arg, state, "MERGE_RANGE", 0);
            break;

        // verbosity
        case OPT_PRINT_TREE:
            arguments->print_tree = true;
            break;
        case OPT_PRINT_TXG:
            arguments->print_txg = true;
            break;

        // positional args
        case ARGP_KEY_ARG:
            if (state->argc != state->next)
                _parse_peer(&arguments->paddr, arg, state);
            else
                _parse_port(&arguments->pport, arg, state, "peer");
            break;

        // verification
        case ARGP_KEY_END:
            empty_check(arguments->tifname, state, "TUN interface");
            empty_check(arguments->bifname, state, "Bind interface");
            null_check(arguments->paddr, state, "At least one destination");
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

void print_grouping_args(struct arguments *args)
{
    printf("Grouping params:\n"
           "MAX addrs:\t%zu\n"
           "OK  addrs:\t%zu\n"
           "MIN leafs:\t%zu\n"
           "MIN childs:\t%zu\n"
           "Split nodes:\t%d\n"
           "Merge range:\t%zu\n",
           args->max_addrs, args->ok_addrs,
           args->min_leafs, args->min_routers,
           args->split_nodes, args->merge_range);
}


static struct argp argp = { .options = options, .parser = parse_opt,
                            .args_doc = args_doc, .doc = doc };

void set_default_args(struct arguments *args)
{
    struct arguments tmp = {
        // network
        .bifname = NULL,
        .bport   = 9999,
        .pport   = 0,
        .paddr   = NULL,

        // discovery
        .dcvr_int  = { .it_interval = { .tv_sec = 5, .tv_nsec = 0 },
                       .it_value    = { .tv_sec = 5, .tv_nsec = 0 }},
        .dcvr_tout = { .it_interval = { .tv_sec = 2, .tv_nsec = 0 },
                       .it_value    = { .tv_sec = 0, .tv_nsec = 0 }},

        // grouping
        .max_addrs   = 10,
        .ok_addrs    =  8,
        .min_leafs   =  2,
        .min_routers =  1,
        .split_nodes =  0,
        .merge_range =  0,

        // verbosity
        .print_tree = false,
        .print_txg  = false
    };

    tmp.tifname = malloc(IFNAMSIZ);
    strncpy(tmp.tifname, "tun1", IFNAMSIZ);
    inet_pton(AF_INET6, "fd15::1", &tmp.taddr);
    inet_pton(AF_INET6, "::", &tmp.baddr);
    memcpy(args, &tmp, sizeof(*args));
}

void init_args(struct arguments *args, int argc, char *argv[])
{
    set_default_args(args);
    argp_parse(&argp, argc, argv, 0, 0, args);
    print_grouping_args(args);
}
