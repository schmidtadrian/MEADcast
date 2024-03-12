#ifndef DISCOVER_H
#define DISCOVER_H

#include <stddef.h>
#include <time.h>

struct dx_targs {
    int fd;
    struct itimerspec *tout;
    struct itimerspec *tint;
    size_t evlen;
};

int start_dx(struct dx_targs *args);

#endif // !DISCOVER_H
