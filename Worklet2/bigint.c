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

int bigint_reserve(BigInteger *a, size_t min_cap) {
    if (!a) return 0;
    if (a->capacity >= min_cap) return 1;

    size_t new_cap = a->capacity == 0 ? 1 : a->capacity;
    while (new_cap < min_cap) {
        new_cap *= 2;
    }

    uint32_t *new_digits = (uint32_t *)realloc(a->digits, new_cap * sizeof(uint32_t));
    if (!new_digits) return 0;

    /* Zero out newly allocated limbs */
    memset(new_digits + a->capacity, 0, (new_cap - a->capacity) * sizeof(uint32_t));

    a->digits = new_digits;
    a->capacity = new_cap;
    return 1;
}

int bigint_resize(BigInteger *a, size_t new_size) {
    if (!a) return 0;
    if (new_size == 0) new_size = 1;
    if (!bigint_reserve(a, new_size)) return 0;
    if (new_size > a->size) {
        memset(a->digits + a->size, 0, (new_size - a->size) * sizeof(uint32_t));
    }
    a->size = new_size;
    return 1;
}

void bigint_clear(BigInteger *a) {
    if (!a) return;
    if (a->digits && a->capacity > 0) {
        memset(a->digits, 0, a->capacity * sizeof(uint32_t));
    }
    a->sign = 0;
    a->size = 1;
}