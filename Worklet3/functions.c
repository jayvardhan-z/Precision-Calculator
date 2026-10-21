// ============================================================================
// File: Worklet3/functions.c
// Description: Arbitrary-precision transcendental, scientific, and mathematical
//              function implementations on BigDecimal objects.
//
// Overview:
// This module implements arbitrary-precision mathematical constants and
// scientific functions declared in functions.h:
// - Precomputed high-precision constants: pi, e, and ln(10) (accurate to 120+ digits).
// - Newton-Raphson iteration: Fast quadratically converging square root (sqrt).
// - Range reduction & Taylor series: Exponential function (exp) and natural log (ln).
// - Modulo 2*pi reduction & Maclaurin series: Trigonometric sine (sin) and cosine (cos).
//
// Precision Management:
// Functions utilize extra guard precision digits (precision + 8 to precision + 12)
// to prevent round-off drift across successive series terms and Newton iterations,
// rounding to user-requested precision with ROUND_HALF_UP upon completion.
// ============================================================================

#define _POSIX_C_SOURCE 200809L

#include "functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// High-Precision Mathematical Constant Strings (120+ decimal places)
// ============================================================================

// Precomputed ratio of a circle's circumference to its diameter (pi)
static const char *PI_STR_120 =
    "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647";

// Precomputed base of the natural logarithm (Euler's number e)
static const char *E_STR_120 =
    "2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427427466391932003059921";

// Precomputed natural logarithm of 10 (ln(10)), used for base-10 range reductions
static const char *LN10_STR_120 =
    "2.302585092994045684017991454684364207601101488628772976033327900967572609677352480235997205089598298341967784042286248633";

// Retrieves constant pi rounded to the requested decimal precision
int math_const_pi(BigDecimal *out, int precision) {
    if (!out) return 0;
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed representation
    if (!decimal_from_string(out, PI_STR_120)) return 0;

    // Round down to requested precision digits if string scale exceeds target
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// Retrieves Euler's number e rounded to the requested decimal precision
int math_const_e(BigDecimal *out, int precision) {
    if (!out) return 0;
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed representation
    if (!decimal_from_string(out, E_STR_120)) return 0;

    // Round to requested precision digits
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// Retrieves ln(10) rounded to the requested decimal precision
int math_const_ln10(BigDecimal *out, int precision) {
    if (!out) return 0;
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed representation
    if (!decimal_from_string(out, LN10_STR_120)) return 0;

    // Round to requested precision digits
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// ============================================================================
// Helper Utilities
// ============================================================================

// Computes the absolute value of a BigDecimal in-place or into destination
static int decimal_abs(BigDecimal *dest, const BigDecimal *src) {
    if (!dest || !src) return 0;

    // If destination is a distinct object, make an exact copy first
    if (dest != src) {
        decimal_free(dest);
        if (!decimal_copy(dest, src)) return 0;
    }

    // Force sign to positive (+1)
    if (dest->significand.sign < 0) {
        dest->significand.sign = 1;
    }
    return 1;
}

// ============================================================================
// Square Root: Newton-Raphson Iteration
// ============================================================================

// Computes sqrt(x) using arbitrary-precision Newton-Raphson iteration:
//   y_{k+1} = 0.5 * (y_k + x / y_k)
// Terminating when |y_{k+1} - y_k| <= 10^(-precision)
int fn_sqrt(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Domain validation: Square root of negative number is undefined over reals
    if (x->significand.sign < 0) {
        return 0;
    }

    // Special case: sqrt(0) = 0
    if (decimal_is_zero(x)) {
        bigint_clear(&out->significand);
        out->scale = 0;
        return 1;
    }

    // Allocate 8 additional guard digits during iteration to avoid intermediate drift
    int guard_prec = precision + 8;

    // Obtain rapid initial estimate y_0 by converting to IEEE 754 double
    char *s = decimal_to_string(x);
    if (!s) return 0;
    double dval = atof(s);
    free(s);

    // Initial floating-point seed guess
    double guess_d = (dval > 0.0) ? sqrt(dval) : 1.0;
    char guess_str[64];
    snprintf(guess_str, sizeof(guess_str), "%.12g", guess_d);

    // Initialize iteration registers
    BigDecimal y, next_y, div_val, sum_val, diff;
    decimal_init(&y);
    decimal_init(&next_y);
    decimal_init(&div_val);
    decimal_init(&sum_val);
    decimal_init(&diff);

    // Parse starting guess into BigDecimal
    if (!decimal_from_string(&y, guess_str)) {
        decimal_free(&y);
        decimal_free(&next_y);
        decimal_free(&div_val);
        decimal_free(&sum_val);
        decimal_free(&diff);
        return 0;
    }

    // Divisor constant: 2.0
    BigDecimal two;
    decimal_init(&two);
    decimal_from_string(&two, "2");

    // Stopping criterion threshold: epsilon = 10^(-precision)
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = precision;

    // Newton-Raphson iteration loop (quadratically doubles correct digits each step)
    int max_iters = 80;
    for (int iter = 0; iter < max_iters; iter++) {
        // Step 1: div_val = x / y_k
        if (!decimal_div(&div_val, x, &y, guard_prec)) break;

        // Step 2: sum_val = y_k + (x / y_k)
        if (!decimal_add(&sum_val, &y, &div_val)) break;

        // Step 3: next_y = sum_val / 2 = 0.5 * (y_k + x / y_k)
        if (!decimal_div(&next_y, &sum_val, &two, guard_prec)) break;

        // Step 4: Compute convergence delta: diff = |next_y - y_k|
        if (!decimal_sub(&diff, &next_y, &y)) break;
        decimal_abs(&diff, &diff);

        // Advance iteration state: y_k = next_y
        decimal_copy(&y, &next_y);

        // Terminate once change is smaller than target precision threshold
        if (decimal_compare(&diff, &epsilon) <= 0) {
            break;
        }
    }

    // Round final estimate to target precision
    decimal_round(&y, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &y);

    // Release temporary allocation registers
    decimal_free(&y);
    decimal_free(&next_y);
    decimal_free(&div_val);
    decimal_free(&sum_val);
    decimal_free(&diff);
    decimal_free(&two);
    decimal_free(&epsilon);
    return 1;
}

// ============================================================================
// Exponential Function: exp(x) = e^x
// ============================================================================

// Computes exp(x) via range splitting:
//   x = m + f (where m = floor(x) is an integer, 0 <= f < 1 is fractional)
//   exp(x) = e^m * sum_{k=0}^N [ f^k / k! ]
int fn_exp(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Special case: e^0 = 1
    if (decimal_is_zero(x)) {
        decimal_from_string(out, "1");
        decimal_round(out, precision, ROUND_HALF_UP);
        return 1;
    }

    // 12 extra guard digits for series accumulation
    int guard_prec = precision + 12;

    // Handle negative inputs via reciprocal identity: exp(-x) = 1 / exp(x)
    if (x->significand.sign < 0) {
        BigDecimal pos_x, exp_pos, one;
        decimal_init(&pos_x);
        decimal_init(&exp_pos);
        decimal_init(&one);

        // Make positive copy: pos_x = |x|
        decimal_copy(&pos_x, x);
        pos_x.significand.sign = 1;
        decimal_from_string(&one, "1");

        // Recursively compute exp(|x|)
        if (!fn_exp(&exp_pos, &pos_x, guard_prec)) {
            decimal_free(&pos_x);
            decimal_free(&exp_pos);
            decimal_free(&one);
            return 0;
        }

        // out = 1 / exp(|x|)
        if (!decimal_div(out, &one, &exp_pos, precision)) {
            decimal_free(&pos_x);
            decimal_free(&exp_pos);
            decimal_free(&one);
            return 0;
        }

        decimal_round(out, precision, ROUND_HALF_UP);
        decimal_free(&pos_x);
        decimal_free(&exp_pos);
        decimal_free(&one);
        return 1;
    }

    // Split positive x into integer part m and fractional part f
    BigDecimal m_dec, f_dec;
    decimal_init(&m_dec);
    decimal_init(&f_dec);
    decimal_copy(&m_dec, x);
    decimal_round(&m_dec, 0, ROUND_TRUNC); // Truncate fractional digits to get integer part
    decimal_sub(&f_dec, x, &m_dec);        // f = x - m (0 <= f < 1)

    // Compute e_pow_m = e^m via binary exponentiation
    BigDecimal e_const, e_pow_m;
    decimal_init(&e_const);
    decimal_init(&e_pow_m);
    math_const_e(&e_const, guard_prec);
    decimal_from_string(&e_pow_m, "1");

    char *m_str = decimal_to_string(&m_dec);
    if (m_str) {
        long m_val = atol(m_str);
        free(m_str);

        // Guard against exponent overflow
        if (m_val > 5000) {
            decimal_free(&m_dec);
            decimal_free(&f_dec);
            decimal_free(&e_const);
            decimal_free(&e_pow_m);
            return 0;
        }

        // Binary repeated squaring algorithm: O(log m) multiplications
        BigDecimal base, acc;
        decimal_init(&base);
        decimal_init(&acc);
        decimal_copy(&base, &e_const);
        decimal_copy(&acc, &e_pow_m);

        long p = m_val;
        while (p > 0) {
            if (p & 1) {
                decimal_mul(&acc, &acc, &base);
                decimal_round(&acc, guard_prec, ROUND_HALF_UP);
            }
            p >>= 1;
            if (p > 0) {
                decimal_mul(&base, &base, &base);
                decimal_round(&base, guard_prec, ROUND_HALF_UP);
            }
        }
        decimal_swap(&e_pow_m, &acc);
        decimal_free(&base);
        decimal_free(&acc);
    }

    // Taylor series expansion for e^f where 0 <= f < 1:
    //   sum_{k=0}^N [ f^k / k! ] = 1 + f + f^2/2! + f^3/3! + ...
    BigDecimal sum, term, k_dec, tmp_term;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&k_dec);
    decimal_init(&tmp_term);

    // Initial term_0 = 1.0, sum = 1.0
    decimal_from_string(&sum, "1");
    decimal_from_string(&term, "1");

    // Dynamic series termination threshold
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    for (int k = 1; k < 200; k++) {
        // term_k = term_{k-1} * f / k
        decimal_mul(&tmp_term, &term, &f_dec);
        char k_buf[32];
        snprintf(k_buf, sizeof(k_buf), "%d", k);
        decimal_from_string(&k_dec, k_buf);
        decimal_div(&term, &tmp_term, &k_dec, guard_prec);

        // Accumulate into sum
        decimal_add(&sum, &sum, &term);

        // Check if term magnitude has dropped below epsilon threshold
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;
    }

    // Combine results: res = e^m * sum(e^f)
    BigDecimal res;
    decimal_init(&res);
    decimal_mul(&res, &e_pow_m, &sum);
    decimal_round(&res, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &res);

    // Clean up temporary variables
    decimal_free(&m_dec);
    decimal_free(&f_dec);
    decimal_free(&e_const);
    decimal_free(&e_pow_m);
    decimal_free(&sum);
    decimal_free(&term);
    decimal_free(&k_dec);
    decimal_free(&tmp_term);
    decimal_free(&epsilon);
    decimal_free(&res);
    return 1;
}

// ============================================================================
// Natural Logarithm: ln(x) for x > 0
// ============================================================================

// Computes ln(x) using scientific range reduction:
//   x = y * 10^k, with y in [0.5, 5.0)
//   ln(x) = k * ln(10) + ln(y)
// and transformed hyperbolic arctangent series:
//   ln(y) = 2 * sum_{i=0}^N [ (1 / (2i + 1)) * u^(2i + 1) ], where u = (y - 1)/(y + 1)
int fn_ln(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Domain validation: ln(x) is undefined for x <= 0 over the real numbers
    if (x->significand.sign <= 0 || decimal_is_zero(x)) {
        return 0;
    }

    int guard_prec = precision + 12;

    // Approximate decimal exponent via double conversion to scale y into [0.5, 5.0)
    char *s = decimal_to_string(x);
    if (!s) return 0;
    double dval = atof(s);
    free(s);

    int k_pow10 = (int)floor(log10(dval));

    BigDecimal y, scale_factor, one;
    decimal_init(&y);
    decimal_init(&scale_factor);
    decimal_init(&one);
    decimal_from_string(&one, "1");

    // Perform exact power-of-10 scaling on significand
    if (k_pow10 >= 0) {
        // x >= 1: Divide by 10^k
        bigint_set_u64(&scale_factor.significand, 1);
        bigint_mul_pow10(&scale_factor.significand, &scale_factor.significand, (size_t)k_pow10);
        scale_factor.scale = 0;
        decimal_div(&y, x, &scale_factor, guard_prec);
    } else {
        // x < 1: Multiply by 10^(-k)
        bigint_set_u64(&scale_factor.significand, 1);
        bigint_mul_pow10(&scale_factor.significand, &scale_factor.significand, (size_t)(-k_pow10));
        scale_factor.scale = 0;
        decimal_mul(&y, x, &scale_factor);
        decimal_round(&y, guard_prec, ROUND_HALF_UP);
    }

    // Transform argument: u = (y - 1) / (y + 1)
    BigDecimal u, y_minus_1, y_plus_1;
    decimal_init(&u);
    decimal_init(&y_minus_1);
    decimal_init(&y_plus_1);

    decimal_sub(&y_minus_1, &y, &one);
    decimal_add(&y_plus_1, &y, &one);
    decimal_div(&u, &y_minus_1, &y_plus_1, guard_prec);

    // Compute u^2 for stepping powers: u^(2i+1) = u^(2i-1) * u^2
    BigDecimal u2, u_pow;
    decimal_init(&u2);
    decimal_init(&u_pow);
    decimal_mul(&u2, &u, &u);
    decimal_round(&u2, guard_prec, ROUND_HALF_UP);
    decimal_copy(&u_pow, &u);

    // Sum series: sum_{i=0}^N [ (1 / (2i + 1)) * u^(2i + 1) ]
    BigDecimal series_sum, term, denom_dec;
    decimal_init(&series_sum);
    decimal_init(&term);
    decimal_init(&denom_dec);

    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    for (int i = 0; i < 200; i++) {
        int denom = 2 * i + 1;
        char denom_str[32];
        snprintf(denom_str, sizeof(denom_str), "%d", denom);
        decimal_from_string(&denom_dec, denom_str);

        // term = u_pow / (2i + 1)
        decimal_div(&term, &u_pow, &denom_dec, guard_prec);
        decimal_add(&series_sum, &series_sum, &term);

        // Check convergence against epsilon
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;

        // Step odd power of u: u_pow *= u^2
        decimal_mul(&u_pow, &u_pow, &u2);
        decimal_round(&u_pow, guard_prec, ROUND_HALF_UP);
    }

    // Compute ln(y) = 2 * series_sum
    BigDecimal ln_y, two;
    decimal_init(&ln_y);
    decimal_init(&two);
    decimal_from_string(&two, "2");
    decimal_mul(&ln_y, &two, &series_sum);
    decimal_round(&ln_y, guard_prec, ROUND_HALF_UP);

    // Compute base-10 shift component: k * ln(10)
    BigDecimal k_ln10, ln10;
    decimal_init(&k_ln10);
    decimal_init(&ln10);
    math_const_ln10(&ln10, guard_prec);

    char k_buf[32];
    snprintf(k_buf, sizeof(k_buf), "%d", k_pow10);
    BigDecimal k_dec;
    decimal_init(&k_dec);
    decimal_from_string(&k_dec, k_buf);

    decimal_mul(&k_ln10, &k_dec, &ln10);

    // Combine: total = ln(y) + k * ln(10)
    BigDecimal total;
    decimal_init(&total);
    decimal_add(&total, &ln_y, &k_ln10);
    decimal_round(&total, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &total);

    // Clean up temporary variables
    decimal_free(&y);
    decimal_free(&scale_factor);
    decimal_free(&one);
    decimal_free(&u);
    decimal_free(&y_minus_1);
    decimal_free(&y_plus_1);
    decimal_free(&u2);
    decimal_free(&u_pow);
    decimal_free(&series_sum);
    decimal_free(&term);
    decimal_free(&denom_dec);
    decimal_free(&epsilon);
    decimal_free(&ln_y);
    decimal_free(&two);
    decimal_free(&k_ln10);
    decimal_free(&ln10);
    decimal_free(&k_dec);
    decimal_free(&total);
    return 1;
}

// ============================================================================
// Trigonometric Helpers: Range Reduction
// ============================================================================

// Reduces input angle x modulo 2*pi to the primary interval [-pi, pi]:
//   x_red = x - round(x / (2*pi)) * (2*pi)
static int reduce_angle_mod_2pi(BigDecimal *x_red, const BigDecimal *x, int guard_prec) {
    BigDecimal pi_val, two_pi;
    decimal_init(&pi_val);
    decimal_init(&two_pi);

    // Obtain 2*pi with guard precision
    math_const_pi(&pi_val, guard_prec);
    BigDecimal two;
    decimal_init(&two);
    decimal_from_string(&two, "2");
    decimal_mul(&two_pi, &pi_val, &two);
    decimal_round(&two_pi, guard_prec, ROUND_HALF_UP);

    // Compute nearest integer multiple of full revolutions: k = round(x / (2*pi))
    BigDecimal div_res, k_round, k_two_pi;
    decimal_init(&div_res);
    decimal_init(&k_round);
    decimal_init(&k_two_pi);

    decimal_div(&div_res, x, &two_pi, guard_prec);
    decimal_copy(&k_round, &div_res);
    decimal_round(&k_round, 0, ROUND_HALF_UP);

    // Subtract integer revolutions: x_red = x - k * (2*pi)
    decimal_mul(&k_two_pi, &k_round, &two_pi);
    decimal_sub(x_red, x, &k_two_pi);
    decimal_round(x_red, guard_prec, ROUND_HALF_UP);

    decimal_free(&pi_val);
    decimal_free(&two_pi);
    decimal_free(&two);
    decimal_free(&div_res);
    decimal_free(&k_round);
    decimal_free(&k_two_pi);
    return 1;
}

// ============================================================================
// Sine: sin(x)
// ============================================================================

// Computes sin(x) using Maclaurin series expansion on reduced angle:
//   sin(x) = sum_{k=0}^N [ (-1)^k * x^(2k + 1) / (2k + 1)! ]
//          = x - x^3/3! + x^5/5! - x^7/7! + ...
int fn_sin(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Special case: sin(0) = 0
    if (decimal_is_zero(x)) {
        bigint_clear(&out->significand);
        out->scale = 0;
        return 1;
    }

    int guard_prec = precision + 12;

    // Reduce input angle modulo 2*pi into [-pi, pi]
    BigDecimal x_red;
    decimal_init(&x_red);
    reduce_angle_mod_2pi(&x_red, x, guard_prec);

    // Precompute x^2 for stepping terms: x^(2k+1) = x^(2k-1) * x^2
    BigDecimal x2;
    decimal_init(&x2);
    decimal_mul(&x2, &x_red, &x_red);
    decimal_round(&x2, guard_prec, ROUND_HALF_UP);

    // Initial values: term_0 = x_red, sum = x_red
    BigDecimal sum, term, tmp_term, denom_dec;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&tmp_term);
    decimal_init(&denom_dec);

    decimal_copy(&sum, &x_red);
    decimal_copy(&term, &x_red);

    // Dynamic series termination threshold
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    // Maclaurin series iteration
    for (int k = 1; k < 150; k++) {
        // term_k = -term_{k-1} * x^2 / ((2k) * (2k + 1))
        decimal_mul(&tmp_term, &term, &x2);

        long long denom = (long long)(2 * k) * (long long)(2 * k + 1);
        char d_buf[32];
        snprintf(d_buf, sizeof(d_buf), "%lld", denom);
        decimal_from_string(&denom_dec, d_buf);

        decimal_div(&term, &tmp_term, &denom_dec, guard_prec);
        term.significand.sign = -term.significand.sign; // Alternating series sign factor (-1)^k

        // Accumulate into sum
        decimal_add(&sum, &sum, &term);

        // Check if term magnitude is smaller than epsilon threshold
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;
    }

    // Round final value to target precision
    decimal_round(&sum, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &sum);

    // Clean up temporary variables
    decimal_free(&x_red);
    decimal_free(&x2);
    decimal_free(&sum);
    decimal_free(&term);
    decimal_free(&tmp_term);
    decimal_free(&denom_dec);
    decimal_free(&epsilon);
    return 1;
}

// ============================================================================
// Cosine: cos(x)
// ============================================================================

// Computes cos(x) using Maclaurin series expansion on reduced angle:
//   cos(x) = sum_{k=0}^N [ (-1)^k * x^(2k) / (2k)! ]
//          = 1 - x^2/2! + x^4/4! - x^6/6! + ...
int fn_cos(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Special case: cos(0) = 1
    if (decimal_is_zero(x)) {
        decimal_from_string(out, "1");
        decimal_round(out, precision, ROUND_HALF_UP);
        return 1;
    }

    int guard_prec = precision + 12;

    // Reduce input angle modulo 2*pi into [-pi, pi]
    BigDecimal x_red;
    decimal_init(&x_red);
    reduce_angle_mod_2pi(&x_red, x, guard_prec);

    // Precompute x^2 for stepping terms: x^(2k) = x^(2k-2) * x^2
    BigDecimal x2;
    decimal_init(&x2);
    decimal_mul(&x2, &x_red, &x_red);
    decimal_round(&x2, guard_prec, ROUND_HALF_UP);

    // Initial values: term_0 = 1.0, sum = 1.0
    BigDecimal sum, term, tmp_term, denom_dec;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&tmp_term);
    decimal_init(&denom_dec);

    decimal_from_string(&sum, "1");
    decimal_from_string(&term, "1");

    // Dynamic series termination threshold
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    // Maclaurin series iteration
    for (int k = 1; k < 150; k++) {
        // term_k = -term_{k-1} * x^2 / ((2k - 1) * (2k))
        decimal_mul(&tmp_term, &term, &x2);

        long long denom = (long long)(2 * k - 1) * (long long)(2 * k);
        char d_buf[32];
        snprintf(d_buf, sizeof(d_buf), "%lld", denom);
        decimal_from_string(&denom_dec, d_buf);

        decimal_div(&term, &tmp_term, &denom_dec, guard_prec);
        term.significand.sign = -term.significand.sign; // Alternating series sign factor (-1)^k

        // Accumulate into sum
        decimal_add(&sum, &sum, &term);

        // Check if term magnitude is smaller than epsilon threshold
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;
    }

    // Round final value to target precision
    decimal_round(&sum, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &sum);

    // Clean up temporary variables
    decimal_free(&x_red);
    decimal_free(&x2);
    decimal_free(&sum);
    decimal_free(&term);
    decimal_free(&tmp_term);
    decimal_free(&denom_dec);
    decimal_free(&epsilon);
    return 1;
}