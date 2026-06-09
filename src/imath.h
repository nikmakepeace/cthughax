/*
 * some helping routines for integer mathematics
 */
#ifndef __IMATH_H
#define __IMATH_H

#include <math.h>

#ifndef M_PI2
#define M_PI2 6.2831385307
#endif

long longSqrt(long x);
int ilog2(int n);

static inline int max(int a, int b) { return (a < b) ? b : a; }
static inline int min(int a, int b) { return (a < b) ? a : b; }

/* x << y also handling negative y */
static inline int shift(int x, int y) { return (y == 0) ? x : (y > 0) ? (x << y) : (x >> (-y)); }

/* mod, handling negative arguments and zero modulo */
static inline int mod(int a, int m) {
    if (m <= 0)
        return a;
    a = a % m;
    if (a < 0)
        a += m;
    return a;
}

/* switch byte order in a word */
static inline int switch_byte_order(int x) {
    unsigned char* cp = (unsigned char*)&x;
    unsigned char c;
    c = cp[0];
    cp[0] = cp[3];
    cp[3] = c;
    c = cp[1];
    cp[1] = cp[2];
    cp[2] = c;
    return x;
}

#endif
