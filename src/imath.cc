/*
 * this file should stay a C file
 */
#include "imath.h"

#include <math.h>

/*
 * Square root for integers.
 * Returns floor(sqrt(n)).
 */
long longSqrt(long n) {
    long root = 0;
    long bit = 0x00008000;

    while (bit) {
        if ((root | bit) * (root | bit) <= n)
            root |= bit;
        bit >>= 1;
    }
    return root;
}

int ilog2(int n) {
    int r = 0;
    while ((1 << r) < n) {
        r++;
    }
    return r;
}
