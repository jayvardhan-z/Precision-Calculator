#include "bigint.h"
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Lifecycle, Normalization & Memory Management
 * ------------------------------------------------------------------------- */

void bigint_init(BigInteger *a) {
    if (!a) return;
    a->sign = 0;
    a->size = 1;
    a->capacity = 1;
    a->digits = (uint32_t *)calloc(1, sizeof(uint32_t));
}

int bigint_init_capacity(BigInteger *a, size_t cap) {
    if (!a) return 0;
    if (cap == 0) cap = 1;
    a->sign = 0;
    a->size = 1;
    a->capacity = cap;
    a->digits = (uint32_t *)calloc(cap, sizeof(uint32_t));
    return (a->digits != NULL);
}
