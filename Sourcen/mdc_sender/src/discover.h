#ifndef DISCOVER_H
#define DISCOVER_H

#include <stddef.h>
#include <time.h>

struct dx_targs {
    int fd;
    long tout;
    long tint;
    size_t evlen;
};

int start_dx(struct dx_targs *args);

static int tout_fd;
static int tint_fd;
static struct timespec tout_ts;
static struct timespec tint_ts;

#endif // !DISCOVER_H
