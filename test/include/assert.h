#ifndef CLAUDEC99_ASSERT_H
#define CLAUDEC99_ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#include <stdio.h>
#include <stdlib.h>
#define assert(expr) \
    ((expr) ? (void)0 : (fprintf(stderr, \
        "assertion failed: %s, file %s, line %d\n", \
        #expr, __FILE__, __LINE__), abort(), (void)0))
#endif

#endif
