#ifndef LWI_PRIVATE_H
#define LWI_PRIVATE_H

#include <lwi.h>
#include "macros.h"

typedef struct lwi_context_t lwi_context_t;

struct lwi_context_t {
    unsigned flags;
    unsigned window_count;  /* number of windows */
    int exitcode;
};

static const unsigned LWI_FLAG_EXIT = 0x1;

extern int __lwi_debug;

#ifndef LWI
    extern lwi_context_t __lwi_context;
    #define LWI __lwi_context
#endif

#define NOTICE(fmt, args...) do {               \
if (__lwi_debug)                                \
    fprintf(stderr, "LWI: " fmt "\n", ##args);  \
} while (0)

#endif /* LWI_PRIVATE_H */
