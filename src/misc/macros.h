/*
  µtil/macros.h
  Copyright (c) 2005-2006 Luke McCarthy
*/

#ifndef MUTIL_MACROS_H
#define MUTIL_MACROS_H

#include <stddef.h>  /* for size_t */

/* Annotations. */
#define IN
#define OUT
#define INOUT

/* Compiler-specific. */
#if defined(__GNUC__)
    #define EXPORT __attribute__((visibility("default")))
    #define PURE __attribute__((const))
    #define NORETURN __attribute__((noreturn))
    #define UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
    #define EXPORT __declspec(dllexport)
    #define PURE
    #define NORETURN __declspec(noreturn)
    #define UNUSED
#endif

/* Inlining. */
#if defined(__GNUC__)
    #if __STDC_VERSION__ < 199901L
        #define inline __inline__
    #endif
    #define FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define inline __inline
    #define FORCE_INLINE __inline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
    #define inline
    #define FORCE_INLINE
#endif

/* Optimisation hints. */
#if defined(__GNUC__)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

/* Offset of struct members. */
#ifndef offsetof
    #define offsetof(type, memb) ((size_t) &((type *)0)->memb)
#endif
#define structof(type, field, ptr) \
    ((type *) (((char *) (ptr)) - offsetof(type, field)))

/* Bits and bytes. */
#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))

#endif /* MUTIL_MACROS_H */
