#ifndef DECIMAL_H
#define DECIMAL_H

#include "bigint.h"
#include "fast_math.h"

// ============================================================================
// File: decimal.h
// Purpose: Fixed-point / scaled arbitrary-precision decimal number engine.
// Role:
//   Represents exact decimal values without IEEE-754 binary floating-point
//   rounding errors.
//
// Mathematical Invariant:
//   Value = significand * 10^(-scale)
//   Example: 123.456 is stored as significand = 123456, scale = 3.
//   Canonical Zero: significand = 0, scale = 0.
// ============================================================================

// Rounding algorithms supported by decimal_round
typedef enum {
    ROUND_HALF_UP, // Standard commercial rounding: ties (>= 0.5) round away from zero
    ROUND_DOWN,    // Floor rounding towards negative infinity
    ROUND_TRUNC    // Direct truncation towards zero
} RoundingMode;

// Scaled decimal structure
typedef struct {
    BigInteger significand; // Unscaled arbitrary-precision integer digits
    int        scale;       // Non-negative count of decimal places following the point
} BigDecimal;

// ----------------------------------------------------------------------------
// Lifecycle & String I/O
// ----------------------------------------------------------------------------

// Initializes BigDecimal to canonical zero (significand = 0, scale = 0)
void  decimal_init(BigDecimal *d);

// Deallocates limbs of significand and resets scale to 0
void  decimal_free(BigDecimal *d);

// Deep copies the significand and scale from src to dest
int   decimal_copy(BigDecimal *dest, const BigDecimal *src);

// Swaps the contents of two BigDecimal structures in O(1) time
void  decimal_swap(BigDecimal *a, BigDecimal *b);

// Enforces canonical zero invariant: if significand == 0, sets scale = 0
void  decimal_normalize(BigDecimal *d);

// Deserializes a decimal string (supporting optional signs, decimals, and exponents) into BigDecimal
int   decimal_from_string(BigDecimal *d, const char *str);

// Serializes a BigDecimal into a freshly allocated human-readable decimal string
char *decimal_to_string(const BigDecimal *d);

// ----------------------------------------------------------------------------
// Basic Decimal Arithmetic
// ----------------------------------------------------------------------------

// Aligns scales to max(scale_a, scale_b) by multiplying by 10^delta, then adds significands
int  decimal_add(BigDecimal *res, const BigDecimal *a, const BigDecimal *b);

// Aligns scales to max(scale_a, scale_b) by multiplying by 10^delta, then subtracts significands
int  decimal_sub(BigDecimal *res, const BigDecimal *a, const BigDecimal *b);

// Computes product: res_significand = a_significand * b_significand, res_scale = scale_a + scale_b
int  decimal_mul(BigDecimal *res, const BigDecimal *a, const BigDecimal *b);

// Computes quotient res = a / b to the requested fractional decimal precision
int  decimal_div(BigDecimal *res, const BigDecimal *a, const BigDecimal *b, int precision);

// Rounds a BigDecimal to target_scale decimal places using the specified RoundingMode
int  decimal_round(BigDecimal *d, int target_scale, RoundingMode mode);

// ----------------------------------------------------------------------------
// Comparisons & Predicates
// ----------------------------------------------------------------------------

// Compares two decimals after virtual scale alignment: returns +1 if a > b, -1 if a < b, 0 if a == b
int  decimal_compare(const BigDecimal *a, const BigDecimal *b);

// Checks if decimal equals zero
int  decimal_is_zero(const BigDecimal *d);

// ----------------------------------------------------------------------------
// Power of 10 Scaling Helper
// ----------------------------------------------------------------------------

// Multiplies BigInteger by 10^exp10: res = a * 10^exp10 using limb shifts and small multiplications
int  bigint_mul_pow10(BigInteger *res, const BigInteger *a, size_t exp10);

#endif /* DECIMAL_H */


