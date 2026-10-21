#define _POSIX_C_SOURCE 200809L
#include "decimal.h"
#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ============================================================================
// File: decimal.c
// Purpose: Implementation of arbitrary-precision fixed-point / scaled decimal arithmetic.
// Role:
//   Represents exact real numbers using an unscaled BigInteger significand
//   and a non-negative integer scale factor (value = significand * 10^-scale).
//   Performs scale alignment for addition/subtraction, scale accumulation for
//   multiplication, target-precision dividend scaling for division, and
//   half-up commercial rounding.
// ============================================================================

// Precomputed powers of 10 up to 10^9 (the base modulus) for limb-level multiplication
static const uint32_t POW10[10] = {
    1U, 10U, 100U, 1000U, 10000U, 100000U, 1000000U, 10000000U, 100000000U, 1000000000U
};

// Multiplies BigInteger by 10^exp10: res = a * 10^exp10
// Decomposes the exponent into whole limb shifts (exp10 / 9) and a small power of 10 (exp10 % 9)
int bigint_mul_pow10(BigInteger *res, const BigInteger *a, size_t exp10) {
    if (!res || !a) return 0;
    // Multiplying 0 by 10^exp10 or multiplying by 10^0 is an identity copy
    if (bigint_is_zero(a) || exp10 == 0) {
        return bigint_copy(res, a);
    }

    // Decompose exponent: limb_shift corresponds to powers of BASE (10^9)
    size_t limb_shift = exp10 / BASE_DIGITS;
    size_t rem_exp = exp10 % BASE_DIGITS;

    BigInteger tmp;
    bigint_init(&tmp);

    // Step 1: Multiply by 10^(exp10 % 9) using fast small multiplication
    if (rem_exp > 0) {
        if (!bigint_mul_small(&tmp, a, POW10[rem_exp])) {
            bigint_free(&tmp);
            return 0;
        }
    } else {
        if (!bigint_copy(&tmp, a)) {
            bigint_free(&tmp);
            return 0;
        }
    }

    // Step 2: Shift by (exp10 / 9) limbs to multiply by BASE^(limb_shift)
    if (limb_shift > 0) {
        if (!bigint_shift_limbs(res, &tmp, limb_shift)) {
            bigint_free(&tmp);
            return 0;
        }
    } else {
        bigint_swap(res, &tmp);
    }

    bigint_free(&tmp);
    return 1;
}

// ----------------------------------------------------------------------------
// Lifecycle & Invariants
// ----------------------------------------------------------------------------

// Initializes BigDecimal to canonical zero: significand = 0, scale = 0
void decimal_init(BigDecimal *d) {
    if (!d) return;
    bigint_init(&d->significand);
    d->scale = 0;
}

// Releases heap memory of significand and resets scale to 0
void decimal_free(BigDecimal *d) {
    if (!d) return;
    bigint_free(&d->significand);
    d->scale = 0;
}

// Deep copies significand and scale from src to dest
int decimal_copy(BigDecimal *dest, const BigDecimal *src) {
    if (!dest || !src) return 0;
    if (!bigint_copy(&dest->significand, &src->significand)) return 0;
    dest->scale = src->scale;
    return 1;
}

// Swaps two BigDecimal structures in O(1) time
void decimal_swap(BigDecimal *a, BigDecimal *b) {
    if (!a || !b || a == b) return;
    bigint_swap(&a->significand, &b->significand);
    int tmp_scale = a->scale;
    a->scale = b->scale;
    b->scale = tmp_scale;
}

// Enforces canonical zero invariant: if significand equals 0, force scale to 0
void decimal_normalize(BigDecimal *d) {
    if (!d) return;
    bigint_normalize(&d->significand);
    if (bigint_is_zero(&d->significand)) {
        d->scale = 0;
    }
}

// Tests whether decimal value equals zero
int decimal_is_zero(const BigDecimal *d) {
    if (!d) return 1;
    return bigint_is_zero(&d->significand);
}

// ----------------------------------------------------------------------------
// String Conversions
// ----------------------------------------------------------------------------

// Deserializes a decimal string (e.g. "-123.456e-2") into a normalized BigDecimal
int decimal_from_string(BigDecimal *d, const char *str) {
    if (!d || !str) return 0;

    d->scale = 0;
    bigint_clear(&d->significand);

    // Step 1: Skip leading whitespace
    while (*str && isspace((unsigned char)*str)) str++;
    if (!*str) return 0;

    // Step 2: Parse sign prefix
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Step 3: Scan unscaled digit string and measure fractional scale
    size_t cap = strlen(str) + 16;
    char *digits_buf = (char *)malloc(cap);
    if (!digits_buf) return 0;

    size_t dpos = 0;
    int saw_dot = 0;
    int scale = 0;

    while (*str && (isdigit((unsigned char)*str) || *str == '.')) {
        if (*str == '.') {
            // Reject numbers containing multiple decimal points
            if (saw_dot) {
                free(digits_buf);
                return 0;
            }
            saw_dot = 1;
            str++;
            continue;
        }

        // Accumulate digit character into buffer
        digits_buf[dpos++] = *str;
        // Count fractional digits appearing after the decimal point
        if (saw_dot) {
            scale++;
        }
        str++;
    }

    // If no digits were encountered, input is invalid
    if (dpos == 0) {
        free(digits_buf);
        return 0;
    }
    digits_buf[dpos] = '\0';

    // Step 4: Parse optional scientific exponent: [eE][+-]?[0-9]+
    int exp_val = 0;
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }
        if (!isdigit((unsigned char)*str)) {
            free(digits_buf);
            return 0;
        }
        while (isdigit((unsigned char)*str)) {
            exp_val = exp_val * 10 + (*str - '0');
            str++;
        }
        exp_val *= exp_sign;
    }

    // Step 5: Convert digit buffer into BigInteger significand
    if (!bigint_from_string(&d->significand, digits_buf)) {
        free(digits_buf);
        return 0;
    }
    free(digits_buf);

    d->significand.sign = (bigint_is_zero(&d->significand) ? 0 : sign);

    // Step 6: Adjust scale by subtracting the exponent: scale_final = scale - exp_val
    int final_scale = scale - exp_val;
    if (final_scale < 0) {
        // Exponent is larger than fractional digits (e.g. 1.2e3 -> 12 * 10^2)
        // Multiply significand by 10^(-final_scale) and set scale to 0
        if (!bigint_mul_pow10(&d->significand, &d->significand, (size_t)(-final_scale))) {
            return 0;
        }
        d->scale = 0;
    } else {
        d->scale = final_scale;
    }

    decimal_normalize(d);
    return 1;
}

// Formats a BigDecimal into a standard human-readable decimal string with decimal point
char *decimal_to_string(const BigDecimal *d) {
    if (!d) return NULL;

    // Fast path: zero renders simply as "0"
    if (decimal_is_zero(d)) {
        char *z = (char *)malloc(2);
        if (z) { z[0] = '0'; z[1] = '\0'; }
        return z;
    }

    // Convert significand integer to raw digit string
    char *raw = bigint_to_string(&d->significand);
    if (!raw) return NULL;

    // If scale is 0, the number is an exact integer
    if (d->scale == 0) {
        return raw;
    }

    int is_neg = (d->significand.sign < 0);
    const char *digits = (is_neg ? raw + 1 : raw);
    size_t len = strlen(digits);

    size_t scale = (size_t)d->scale;
    size_t out_cap = len + scale + 8;
    char *out = (char *)malloc(out_cap);
    if (!out) {
        free(raw);
        return NULL;
    }

    size_t pos = 0;
    // Prefix negative sign
    if (is_neg) {
        out[pos++] = '-';
    }

    // Case 1: All digits belong to the fractional part (len <= scale)
    // Example: significand = 45, scale = 4 -> "0.0045"
    if (len <= scale) {
        out[pos++] = '0';
        out[pos++] = '.';
        size_t pad_zeros = scale - len;
        // Insert leading zeros after the decimal point
        for (size_t i = 0; i < pad_zeros; i++) {
            out[pos++] = '0';
        }
        memcpy(out + pos, digits, len);
        pos += len;
    }
    // Case 2: Number has both integer and fractional parts (len > scale)
    // Example: significand = 12345, scale = 2 -> "123.45"
    else {
        size_t int_digits = len - scale;
        memcpy(out + pos, digits, int_digits);
        pos += int_digits;
        out[pos++] = '.';
        memcpy(out + pos, digits + int_digits, scale);
        pos += scale;
    }

    out[pos] = '\0';
    free(raw);
    return out;
}

// ----------------------------------------------------------------------------
// Basic Decimal Arithmetic
// ----------------------------------------------------------------------------

// Adds two decimals: res = a + b
// Aligns operands to the common scale max(scale_a, scale_b) by multiplying by 10^delta
int decimal_add(BigDecimal *res, const BigDecimal *a, const BigDecimal *b) {
    if (!res || !a || !b) return 0;

    int max_scale = (a->scale > b->scale ? a->scale : b->scale);

    // Scale up significands so both have the same virtual scale
    BigInteger a_scaled, b_scaled;
    bigint_init(&a_scaled); bigint_init(&b_scaled);

    if (!bigint_mul_pow10(&a_scaled, &a->significand, (size_t)(max_scale - a->scale)) ||
        !bigint_mul_pow10(&b_scaled, &b->significand, (size_t)(max_scale - b->scale))) {
        bigint_free(&a_scaled); bigint_free(&b_scaled);
        return 0;
    }

    BigDecimal tmp;
    decimal_init(&tmp);

    // Add aligned significands
    if (!bigint_add(&tmp.significand, &a_scaled, &b_scaled)) {
        bigint_free(&a_scaled); bigint_free(&b_scaled);
        decimal_free(&tmp);
        return 0;
    }

    tmp.scale = max_scale;
    decimal_normalize(&tmp);

    decimal_swap(res, &tmp);
    decimal_free(&tmp);
    bigint_free(&a_scaled);
    bigint_free(&b_scaled);
    return 1;
}

// Subtracts two decimals: res = a - b
// Aligns operands to the common scale max(scale_a, scale_b) by multiplying by 10^delta
int decimal_sub(BigDecimal *res, const BigDecimal *a, const BigDecimal *b) {
    if (!res || !a || !b) return 0;

    int max_scale = (a->scale > b->scale ? a->scale : b->scale);

    // Scale up significands to match common scale
    BigInteger a_scaled, b_scaled;
    bigint_init(&a_scaled); bigint_init(&b_scaled);

    if (!bigint_mul_pow10(&a_scaled, &a->significand, (size_t)(max_scale - a->scale)) ||
        !bigint_mul_pow10(&b_scaled, &b->significand, (size_t)(max_scale - b->scale))) {
        bigint_free(&a_scaled); bigint_free(&b_scaled);
        return 0;
    }

    BigDecimal tmp;
    decimal_init(&tmp);

    // Subtract aligned significands
    if (!bigint_sub(&tmp.significand, &a_scaled, &b_scaled)) {
        bigint_free(&a_scaled); bigint_free(&b_scaled);
        decimal_free(&tmp);
        return 0;
    }

    tmp.scale = max_scale;
    decimal_normalize(&tmp);

    decimal_swap(res, &tmp);
    decimal_free(&tmp);
    bigint_free(&a_scaled);
    bigint_free(&b_scaled);
    return 1;
}

// Multiplies two decimals: res = a * b
// Product significand = a_significand * b_significand; product scale = scale_a + scale_b
int decimal_mul(BigDecimal *res, const BigDecimal *a, const BigDecimal *b) {
    if (!res || !a || !b) return 0;

    BigDecimal tmp;
    decimal_init(&tmp);

    // Multiply unscaled integer significands using adaptive multiplier (Schoolbook or Karatsuba)
    if (!bigint_mul(&tmp.significand, &a->significand, &b->significand)) {
        decimal_free(&tmp);
        return 0;
    }

    // Scales add under multiplication
    tmp.scale = a->scale + b->scale;
    decimal_normalize(&tmp);

    decimal_swap(res, &tmp);
    decimal_free(&tmp);
    return 1;
}

// Divides two decimals: res = a / b to requested fractional precision
// Shifts dividend or divisor by powers of 10 such that integer quotient has exact requested scale
int decimal_div(BigDecimal *res, const BigDecimal *a, const BigDecimal *b, int precision) {
    if (!res || !a || !b) return 0;
    if (decimal_is_zero(b)) return 0; // Division by zero error

    if (precision < 0) precision = 0;

    // Mathematical formula for quotient scaling:
    // Q = (a * 10^-scale_a) / (b * 10^-scale_b) * 10^precision
    //   = (a_sig / b_sig) * 10^(scale_b - scale_a + precision)
    int shift = b->scale - a->scale + precision;

    BigInteger num, den;
    bigint_init(&num); bigint_init(&den);

    // Apply shift to numerator if shift >= 0, else apply positive shift to denominator
    if (shift >= 0) {
        if (!bigint_mul_pow10(&num, &a->significand, (size_t)shift) ||
            !bigint_copy(&den, &b->significand)) {
            bigint_free(&num); bigint_free(&den);
            return 0;
        }
    } else {
        if (!bigint_copy(&num, &a->significand) ||
            !bigint_mul_pow10(&den, &b->significand, (size_t)(-shift))) {
            bigint_free(&num); bigint_free(&den);
            return 0;
        }
    }

    // Perform integer division with Knuth Algorithm D
    DivisionResult div_res;
    division_result_init(&div_res);

    if (!bigint_divide(&div_res, &num, &den)) {
        bigint_free(&num); bigint_free(&den);
        division_result_free(&div_res);
        return 0;
    }

    BigDecimal tmp;
    decimal_init(&tmp);
    bigint_swap(&tmp.significand, &div_res.quotient);
    tmp.scale = precision;
    decimal_normalize(&tmp);

    decimal_swap(res, &tmp);

    decimal_free(&tmp);
    division_result_free(&div_res);
    bigint_free(&num);
    bigint_free(&den);
    return 1;
}

// Rounds a BigDecimal to target_scale decimal places
int decimal_round(BigDecimal *d, int target_scale, RoundingMode mode) {
    if (!d || target_scale < 0) return 0;
    // If current scale is already <= target_scale, no rounding needed
    if (d->scale <= target_scale) return 1;

    size_t delta = (size_t)(d->scale - target_scale);

    // Construct divisor = 10^delta
    BigInteger divisor;
    bigint_init(&divisor);
    BigInteger one;
    bigint_init(&one);
    bigint_set_u64(&one, 1);
    bigint_mul_pow10(&divisor, &one, delta);
    bigint_free(&one);

    // Divide significand by 10^delta
    DivisionResult div_res;
    division_result_init(&div_res);

    if (!bigint_divide(&div_res, &d->significand, &divisor)) {
        bigint_free(&divisor);
        division_result_free(&div_res);
        return 0;
    }

    // Handle HALF_UP rounding: round away from zero if 2 * |remainder| >= divisor
    if (mode == ROUND_HALF_UP && !bigint_is_zero(&div_res.remainder)) {
        BigInteger rem_times_2;
        bigint_init(&rem_times_2);
        bigint_mul_small(&rem_times_2, &div_res.remainder, 2);

        // Check condition: 2 * |remainder| >= divisor
        if (bigint_compare_abs(&rem_times_2, &divisor) >= 0) {
            if (d->significand.sign >= 0) {
                bigint_add_small(&div_res.quotient, &div_res.quotient, 1);
            } else {
                bigint_add_small(&div_res.quotient, &div_res.quotient, -1);
            }
        }
        bigint_free(&rem_times_2);
    }

    bigint_swap(&d->significand, &div_res.quotient);
    d->scale = target_scale;
    decimal_normalize(d);

    bigint_free(&divisor);
    division_result_free(&div_res);
    return 1;
}

// Compares two decimals: returns +1 if a > b, -1 if a < b, 0 if a == b
int decimal_compare(const BigDecimal *a, const BigDecimal *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    int max_scale = (a->scale > b->scale ? a->scale : b->scale);
    BigInteger sa, sb;
    bigint_init(&sa); bigint_init(&sb);

    // Align both significands to common scale for exact integer comparison
    bigint_mul_pow10(&sa, &a->significand, (size_t)(max_scale - a->scale));
    bigint_mul_pow10(&sb, &b->significand, (size_t)(max_scale - b->scale));

    int cmp = bigint_compare(&sa, &sb);
    bigint_free(&sa);
    bigint_free(&sb);
    return cmp;
}

