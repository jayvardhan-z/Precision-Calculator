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

void bigint_free(BigInteger *a) {
    if (!a) return;
    if (a->digits) {
        free(a->digits);
        a->digits = NULL;
    }
    a->capacity = 0;
    a->size = 0;
    a->sign = 0;
}

int bigint_copy(BigInteger *dest, const BigInteger *src) {
    if (!dest || !src) return 0;
    if (dest == src) return 1;

    if (!bigint_reserve(dest, src->size)) return 0;
    memcpy(dest->digits, src->digits, src->size * sizeof(uint32_t));
    dest->size = src->size;
    dest->sign = src->sign;
    return 1;
}

void bigint_swap(BigInteger *a, BigInteger *b) {
    if (!a || !b || a == b) return;
    BigInteger tmp = *a;
    *a = *b;
    *b = tmp;
}

void bigint_normalize(BigInteger *a) {
    if (!a || !a->digits || a->size == 0) return;

    /* Strip leading zero limbs */
    while (a->size > 1 && a->digits[a->size - 1] == 0) {
        a->size--;
    }

    /* Enforce canonical zero invariant: size = 1, digits[0] = 0, sign = 0 */
    if (a->size == 1 && a->digits[0] == 0) {
        a->sign = 0;
    } else if (a->sign == 0) {
        /* Default non-zero numbers with undefined sign to +1 */
        a->sign = 1;
    }
}

/* -------------------------------------------------------------------------
 * Value Inspection & Predicates
 * ------------------------------------------------------------------------- */

int bigint_is_zero(const BigInteger *a) {
    if (!a || !a->digits || a->size == 0) return 1;
    return (a->size == 1 && a->digits[0] == 0);
}

int bigint_is_one(const BigInteger *a) {
    if (!a || !a->digits) return 0;
    return (a->sign == 1 && a->size == 1 && a->digits[0] == 1);
}


/* -------------------------------------------------------------------------
 * Comparisons
 * ------------------------------------------------------------------------- */

int bigint_compare_abs(const BigInteger *a, const BigInteger *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    /* Normalize checks on sizes */
    size_t a_size = a->size;
    size_t b_size = b->size;

    while (a_size > 1 && a->digits[a_size - 1] == 0) a_size--;
    while (b_size > 1 && b->digits[b_size - 1] == 0) b_size--;

    if (a_size < b_size) return -1;
    if (a_size > b_size) return 1;

    /* Limb-by-limb check from MSB down to LSB */
    for (size_t i = a_size; i > 0; i--) {
        size_t idx = i - 1;
        if (a->digits[idx] < b->digits[idx]) return -1;
        if (a->digits[idx] > b->digits[idx]) return 1;
    }
    return 0;
}

int bigint_compare(const BigInteger *a, const BigInteger *b) {
    if (!a && !b) return 0;
    if (!a) return (b->sign >= 0 ? -1 : 1);
    if (!b) return (a->sign >= 0 ? 1 : -1);

    int a_zero = bigint_is_zero(a);
    int b_zero = bigint_is_zero(b);

    if (a_zero && b_zero) return 0;
    if (a_zero) return (b->sign > 0 ? -1 : 1);
    if (b_zero) return (a->sign > 0 ? 1 : -1);

    if (a->sign != b->sign) {
        return (a->sign > b->sign ? 1 : -1);
    }

    int abs_cmp = bigint_compare_abs(a, b);
    return (a->sign > 0 ? abs_cmp : -abs_cmp);
}

int bigint_equal(const BigInteger *a, const BigInteger *b) {
    return bigint_compare(a, b) == 0;
}

int bigint_less(const BigInteger *a, const BigInteger *b) {
    return bigint_compare(a, b) < 0;
}

int bigint_greater(const BigInteger *a, const BigInteger *b) {
    return bigint_compare(a, b) > 0;
}

int bigint_less_equal(const BigInteger *a, const BigInteger *b) {
    return bigint_compare(a, b) <= 0;
}

int bigint_greater_equal(const BigInteger *a, const BigInteger *b) {
    return bigint_compare(a, b) >= 0;
}
