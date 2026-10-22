#include "bigint.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// File: bigint.c
// Purpose: Implementation of Base-10^9 arbitrary-precision integer arithmetic.
// Role:
//   Provides fundamental memory allocation, normalization invariants,
//   magnitude comparisons, signed addition/subtraction, scalar multiplication,
//   and schoolbook column multiplication.
// ============================================================================

// ----------------------------------------------------------------------------
// Lifecycle, Normalization & Memory Management
// ----------------------------------------------------------------------------

// Initializes BigInteger to canonical zero: sign = 0, size = 1, capacity = 1, digits[0] = 0
void bigint_init(BigInteger *a) {
    if (!a) return;
    a->sign = 0;
    a->size = 1;
    a->capacity = 1;
    // Allocate single limb zero-initialized via calloc
    a->digits = (uint32_t *)calloc(1, sizeof(uint32_t));
}

// Initializes BigInteger with explicit pre-allocated limb capacity
int bigint_init_capacity(BigInteger *a, size_t cap) {
    if (!a) return 0;
    // Ensure minimum capacity of at least 1 limb
    if (cap == 0) cap = 1;
    a->sign = 0;
    a->size = 1;
    a->capacity = cap;
    // Allocate zeroed limb buffer
    a->digits = (uint32_t *)calloc(cap, sizeof(uint32_t));
    return (a->digits != NULL);
}

// Ensures that the allocated buffer holds at least min_cap limbs
int bigint_reserve(BigInteger *a, size_t min_cap) {
    if (!a) return 0;
    // Fast path: already sufficient capacity
    if (a->capacity >= min_cap) return 1;

    // Geometric growth strategy: double capacity until it satisfies min_cap
    size_t new_cap = a->capacity == 0 ? 1 : a->capacity;
    while (new_cap < min_cap) {
        new_cap *= 2;
    }

    // Reallocate limb array buffer
    uint32_t *new_digits = (uint32_t *)realloc(a->digits, new_cap * sizeof(uint32_t));
    if (!new_digits) return 0; // Allocation failure guard

    // Zero out newly added limb storage to prevent garbage data
    memset(new_digits + a->capacity, 0, (new_cap - a->capacity) * sizeof(uint32_t));

    a->digits = new_digits;
    a->capacity = new_cap;
    return 1;
}

// Resizes active limb count, reserving memory if needed and zeroing new limbs
int bigint_resize(BigInteger *a, size_t new_size) {
    if (!a) return 0;
    // Maintain minimum size of 1
    if (new_size == 0) new_size = 1;
    if (!bigint_reserve(a, new_size)) return 0;

    // If growing, ensure newly exposed limbs are initialized to 0
    if (new_size > a->size) {
        memset(a->digits + a->size, 0, (new_size - a->size) * sizeof(uint32_t));
    }
    a->size = new_size;
    return 1;
}

// Clears value to 0 without releasing allocated memory buffer
void bigint_clear(BigInteger *a) {
    if (!a) return;
    // Zero out existing limb contents
    if (a->digits && a->capacity > 0) {
        memset(a->digits, 0, a->capacity * sizeof(uint32_t));
    }
    // Restore canonical zero invariant
    a->sign = 0;
    a->size = 1;
}

// Deallocates heap memory held by digits array and resets struct fields to 0
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

// Performs a deep copy from src to dest
int bigint_copy(BigInteger *dest, const BigInteger *src) {
    if (!dest || !src) return 0;
    // Guard against self-copy
    if (dest == src) return 1;

    // Ensure destination buffer has sufficient capacity
    if (!bigint_reserve(dest, src->size)) return 0;

    // Copy limb array and metadata
    memcpy(dest->digits, src->digits, src->size * sizeof(uint32_t));
    dest->size = src->size;
    dest->sign = src->sign;
    return 1;
}

// Swaps internal state of two BigInteger variables in O(1) time
void bigint_swap(BigInteger *a, BigInteger *b) {
    if (!a || !b || a == b) return;
    BigInteger tmp = *a;
    *a = *b;
    *b = tmp;
}

// Normalizes BigInteger: removes superfluous leading zero limbs and enforces canonical zero invariant
void bigint_normalize(BigInteger *a) {
    if (!a || !a->digits || a->size == 0) return;

    // Strip leading zero limbs starting from most significant down to index 1
    while (a->size > 1 && a->digits[a->size - 1] == 0) {
        a->size--;
    }

    // Enforce canonical zero invariant: if only limb is zero, sign must be 0
    if (a->size == 1 && a->digits[0] == 0) {
        a->sign = 0;
    } else if (a->sign == 0) {
        // Default non-zero numbers with undefined sign to +1
        a->sign = 1;
    }
}

// ----------------------------------------------------------------------------
// Value Inspection & Predicates
// ----------------------------------------------------------------------------

// Tests whether BigInteger represents 0
int bigint_is_zero(const BigInteger *a) {
    if (!a || !a->digits || a->size == 0) return 1;
    // In normalized form, zero has size 1 and digits[0] == 0
    return (a->size == 1 && a->digits[0] == 0);
}

// Tests whether BigInteger represents positive one (+1)
int bigint_is_one(const BigInteger *a) {
    if (!a || !a->digits) return 0;
    return (a->sign == 1 && a->size == 1 && a->digits[0] == 1);
}

// ----------------------------------------------------------------------------
// Comparisons
// ----------------------------------------------------------------------------

// Compares absolute magnitudes |a| and |b|
// Returns +1 if |a| > |b|, -1 if |a| < |b|, 0 if |a| == |b|
int bigint_compare_abs(const BigInteger *a, const BigInteger *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    // Determine effective size without counting unnormalized leading zeros
    size_t a_size = a->size;
    size_t b_size = b->size;

    while (a_size > 1 && a->digits[a_size - 1] == 0) a_size--;
    while (b_size > 1 && b->digits[b_size - 1] == 0) b_size--;

    // Step 1: Compare number of active limbs (larger limb count implies strictly larger magnitude)
    if (a_size < b_size) return -1;
    if (a_size > b_size) return 1;

    // Step 2: Limb counts are identical -> Compare limb-by-limb from MSB down to LSB
    for (size_t i = a_size; i > 0; i--) {
        size_t idx = i - 1;
        if (a->digits[idx] < b->digits[idx]) return -1;
        if (a->digits[idx] > b->digits[idx]) return 1;
    }
    return 0; // Exactly equal magnitudes
}

// Full signed comparison: returns +1 if a > b, -1 if a < b, 0 if a == b
int bigint_compare(const BigInteger *a, const BigInteger *b) {
    if (!a && !b) return 0;
    if (!a) return (b->sign >= 0 ? -1 : 1);
    if (!b) return (a->sign >= 0 ? 1 : -1);

    int a_zero = bigint_is_zero(a);
    int b_zero = bigint_is_zero(b);

    // Case 1: Both operands are zero
    if (a_zero && b_zero) return 0;
    // Case 2: One operand is zero
    if (a_zero) return (b->sign > 0 ? -1 : 1);
    if (b_zero) return (a->sign > 0 ? 1 : -1);

    // Case 3: Operands have differing signs (positive is always greater than negative)
    if (a->sign != b->sign) {
        return (a->sign > b->sign ? 1 : -1);
    }

    // Case 4: Operands have same sign -> Compare absolute magnitudes
    int abs_cmp = bigint_compare_abs(a, b);
    // If both positive, larger magnitude is larger; if both negative, larger magnitude is smaller
    return (a->sign > 0 ? abs_cmp : -abs_cmp);
}

// Predicate wrappers around bigint_compare
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

// ----------------------------------------------------------------------------
// Magnitude Addition & Subtraction
// ----------------------------------------------------------------------------

// Unsigned magnitude addition: res = |a| + |b|
int bigint_add_abs(BigInteger *res, const BigInteger *a, const BigInteger *b) {
    if (!res || !a || !b) return 0;

    // Result needs at most max(size_a, size_b) + 1 limbs for possible final carry
    size_t max_size = (a->size > b->size ? a->size : b->size);
    BigInteger tmp;
    if (!bigint_init_capacity(&tmp, max_size + 1)) return 0;

    uint64_t carry = 0;
    // Add limbs from least significant (index 0) up to most significant
    for (size_t i = 0; i < max_size; i++) {
        uint64_t sum = carry;
        if (i < a->size) sum += a->digits[i];
        if (i < b->size) sum += b->digits[i];

        // Store remainder modulo BASE in current limb
        tmp.digits[i] = (uint32_t)(sum % BASE);
        // Carry quotient forward to next limb
        carry = sum / BASE;
    }

    // If carry remains after the final limb addition, create an additional limb
    size_t res_size = max_size;
    if (carry > 0) {
        tmp.digits[res_size++] = (uint32_t)carry;
    }
    tmp.size = res_size;
    tmp.sign = 1;
    // Normalize to strip any redundant zero limbs
    bigint_normalize(&tmp);

    // Safely transfer result to destination
    bigint_free(res);
    *res = tmp;
    return 1;
}

// Unsigned magnitude subtraction: res = |a| - |b| (Precondition: |a| >= |b|)
int bigint_sub_abs(BigInteger *res, const BigInteger *a, const BigInteger *b) {
    if (!res || !a || !b) return 0;

    // Temporary storage matching size of larger operand
    BigInteger tmp;
    if (!bigint_init_capacity(&tmp, a->size)) return 0;

    int64_t borrow = 0;
    // Subtract limbs from least significant up to most significant
    for (size_t i = 0; i < a->size; i++) {
        int64_t diff = (int64_t)a->digits[i] - (i < b->size ? (int64_t)b->digits[i] : 0) - borrow;
        // If current difference is negative, borrow BASE from the next higher limb
        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        tmp.digits[i] = (uint32_t)diff;
    }

    tmp.size = a->size;
    tmp.sign = 1;
    // Normalize to strip leading zero limbs generated by cancellation
    bigint_normalize(&tmp);

    bigint_free(res);
    *res = tmp;
    return 1;
}

// ----------------------------------------------------------------------------
// Signed Addition & Subtraction
// ----------------------------------------------------------------------------

// Full signed addition: res = a + b
int bigint_add(BigInteger *res, const BigInteger *a, const BigInteger *b) {
    if (!res || !a || !b) return 0;

    // Identity optimizations: adding zero returns a copy of the other operand
    if (bigint_is_zero(a)) return bigint_copy(res, b);
    if (bigint_is_zero(b)) return bigint_copy(res, a);

    // Case 1: Operands have identical signs -> add magnitudes and preserve sign
    if (a->sign == b->sign) {
        int sign = a->sign;
        if (!bigint_add_abs(res, a, b)) return 0;
        res->sign = sign;
        bigint_normalize(res);
        return 1;
    }

    // Case 2: Operands have differing signs -> compare magnitudes to decide sign and subtraction order
    int cmp = bigint_compare_abs(a, b);
    if (cmp == 0) {
        // Equal magnitudes with opposite signs cancel out to exactly 0
        bigint_clear(res);
        return 1;
    }

    if (cmp > 0) {
        // |a| > |b| -> result = sign(a) * (|a| - |b|)
        int sign = a->sign;
        if (!bigint_sub_abs(res, a, b)) return 0;
        res->sign = sign;
        bigint_normalize(res);
    } else {
        // |b| > |a| -> result = sign(b) * (|b| - |a|)
        int sign = b->sign;
        if (!bigint_sub_abs(res, b, a)) return 0;
        res->sign = sign;
        bigint_normalize(res);
    }
    return 1;
}

// Full signed subtraction: res = a - b (calculated as a + (-b))
int bigint_sub(BigInteger *res, const BigInteger *a, const BigInteger *b) {
    if (!res || !a || !b) return 0;

    // Identity optimizations
    if (bigint_is_zero(b)) return bigint_copy(res, a);
    if (bigint_is_zero(a)) {
        if (!bigint_copy(res, b)) return 0;
        res->sign = -res->sign; // Negate sign of b
        return 1;
    }

    // Compute a - b by inverting sign of b and adding: a + (-b)
    BigInteger b_neg;
    bigint_init(&b_neg);
    if (!bigint_copy(&b_neg, b)) return 0;
    b_neg.sign = -b_neg.sign;

    int ret = bigint_add(res, a, &b_neg);
    bigint_free(&b_neg);
    return ret;
}

// Scalar addition helper: adds a signed 64-bit integer to a BigInteger
int bigint_add_small(BigInteger *res, const BigInteger *a, int64_t val) {
    BigInteger b_val;
    bigint_init(&b_val);
    if (!bigint_set_i64(&b_val, val)) {
        bigint_free(&b_val);
        return 0;
    }
    int ret = bigint_add(res, a, &b_val);
    bigint_free(&b_val);
    return ret;
}

// ----------------------------------------------------------------------------
// Multiplication
// ----------------------------------------------------------------------------

// Multiplies BigInteger by a single limb scalar (val < BASE): res = a * val
int bigint_mul_small(BigInteger *res, const BigInteger *a, uint32_t val) {
    if (!res || !a) return 0;

    // Multiplying by zero results in canonical zero
    if (val == 0 || bigint_is_zero(a)) {
        bigint_clear(res);
        return 1;
    }

    // Allocate temporary with room for 1 carry limb
    BigInteger tmp;
    if (!bigint_init_capacity(&tmp, a->size + 1)) return 0;

    uint64_t carry = 0;
    // Multiply each limb of a by scalar val, propagating carry
    for (size_t i = 0; i < a->size; i++) {
        uint64_t prod = (uint64_t)a->digits[i] * val + carry;
        tmp.digits[i] = (uint32_t)(prod % BASE);
        carry = prod / BASE;
    }

    size_t tmp_size = a->size;
    // Append remaining carry if non-zero
    if (carry > 0) {
        tmp.digits[tmp_size++] = (uint32_t)carry;
    }

    tmp.size = tmp_size;
    tmp.sign = a->sign;
    bigint_normalize(&tmp);

    bigint_swap(res, &tmp);
    bigint_free(&tmp);
    return 1;
}

// Classical schoolbook column multiplication: res = a * b in O(N * M) time
int bigint_mul_schoolbook(BigInteger *res, const BigInteger *a, const BigInteger *b) {
    if (!res || !a || !b) return 0;

    // If either operand is zero, the product is zero
    if (bigint_is_zero(a) || bigint_is_zero(b)) {
        bigint_clear(res);
        return 1;
    }

    // Maximum limbs in product = size_a + size_b
    size_t req_cap = a->size + b->size;
    BigInteger tmp;
    if (!bigint_init_capacity(&tmp, req_cap)) return 0;
    tmp.size = req_cap;

    // Double loop computing cross products and accumulating into limb buffer
    for (size_t i = 0; i < a->size; i++) {
        uint64_t carry = 0;
        uint64_t ai = a->digits[i];

        for (size_t j = 0; j < b->size; j++) {
            // Compute: current_sum + a[i] * b[j] + previous_carry
            // Guaranteed not to overflow uint64_t because (BASE-1)^2 + 2*BASE < 10^18 < 2^64-1
            uint64_t cur = (uint64_t)tmp.digits[i + j] + ai * (uint64_t)b->digits[j] + carry;
            tmp.digits[i + j] = (uint32_t)(cur % BASE);
            carry = cur / BASE;
        }

        // Ripple remaining carry across higher limbs
        size_t k = i + b->size;
        while (carry > 0) {
            uint64_t cur = (uint64_t)tmp.digits[k] + carry;
            tmp.digits[k] = (uint32_t)(cur % BASE);
            carry = cur / BASE;
            k++;
        }
    }

    // Product sign is (+1)*(-1) = -1, or (+1)*(+1) / (-1)*(-1) = +1
    tmp.sign = a->sign * b->sign;
    // Strip leading zeros
    bigint_normalize(&tmp);

    bigint_swap(res, &tmp);
    bigint_free(&tmp);
    return 1;
}

// ----------------------------------------------------------------------------
// Numeric Setter Helpers
// ----------------------------------------------------------------------------

// Sets BigInteger value from an unsigned 64-bit integer
int bigint_set_u64(BigInteger *a, uint64_t val) {
    if (!a) return 0;
    bigint_clear(a);

    if (val == 0) {
        return 1; // Already canonical zero
    }

    a->sign = 1;
    a->size = 0;

    // Extract Base-10^9 limbs from least to most significant
    while (val > 0) {
        if (!bigint_reserve(a, a->size + 1)) return 0;
        a->digits[a->size++] = (uint32_t)(val % BASE);
        val /= BASE;
    }
    bigint_normalize(a);
    return 1;
}

// Sets BigInteger value from a signed 64-bit integer
int bigint_set_i64(BigInteger *a, int64_t val) {
    if (!a) return 0;
    if (val == 0) {
        bigint_clear(a);
        return 1;
    }

    int sign = (val < 0 ? -1 : 1);
    uint64_t uval;
    if (val < 0) {
        // Safe conversion of INT64_MIN avoiding undefined signed overflow
        uval = (uint64_t)(-(val + 1)) + 1;
    } else {
        uval = (uint64_t)val;
    }

    if (!bigint_set_u64(a, uval)) return 0;
    a->sign = sign;
    return 1;
}