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