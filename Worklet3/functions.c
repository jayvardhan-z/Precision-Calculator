#define _POSIX_C_SOURCE 200809L
#include "functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// File: functions.c
// Purpose: Implementation of high-precision mathematical and transcendental functions.
// Role:
//   Computes constants (pi, e, ln10) and transcendental functions (sqrt, exp, ln, sin, cos)
//   operating on arbitrary-precision BigDecimal structures.
// Algorithmic Techniques:
//   1. Guard Precision: Internal calculations employ 8 to 12 extra decimal digits to
//      shield accumulated series terms from round-off noise.
//   2. Range Reduction: Inputs are mapped into optimal convergence domains (e.g.,
//      angles modulo 2*pi, logarithms scaled by powers of 10, exp split into integer/fraction).
//   3. Iterative Numerical Methods: Newton-Raphson iteration for square roots and
//      Maclaurin/Taylor series expansions with dynamic epsilon threshold convergence.
// ============================================================================

// ----------------------------------------------------------------------------
// Static High-Precision Mathematical Constant Tables (120+ decimal places)
// ----------------------------------------------------------------------------

// Precomputed digits of Pi (pi = 3.1415926535...) accurate to 121 decimal places
static const char *PI_STR_120 =
    "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647";

// Precomputed digits of Euler's Number e (e = 2.7182818284...) accurate to 121 decimal places
static const char *E_STR_120 =
    "2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427427466391932003059921";

// Precomputed digits of Natural Logarithm of 10 (ln(10) = 2.3025850929...) accurate to 121 decimal places
static const char *LN10_STR_120 =
    "2.302585092994045684017991454684364207601101488628772976033327900967572609677352480235997205089598298341967784042286248633";

// ----------------------------------------------------------------------------
// Function: math_const_pi
// What: Generates the mathematical constant Pi (pi) rounded to target precision.
// How:
//   1. Validates output pointer and clamps negative precision to 0.
//   2. Deserializes the 120-digit constant string into the destination BigDecimal.
//   3. If the constant has more digits than requested, rounds it using ROUND_HALF_UP.
// ----------------------------------------------------------------------------
int math_const_pi(BigDecimal *out, int precision) {
    // Validate output destination pointer
    if (!out) return 0;

    // Negative precision is clamped to 0 decimal places
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed string into BigDecimal structure
    if (!decimal_from_string(out, PI_STR_120)) return 0;

    // Round down to requested precision if user asked for fewer than 120 places
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// ----------------------------------------------------------------------------
// Function: math_const_e
// What: Generates Euler's constant (e) rounded to target precision.
// How:
//   1. Validates destination pointer and ensures precision is non-negative.
//   2. Deserializes the 120-digit constant string into the destination BigDecimal.
//   3. Applies ROUND_HALF_UP commercial rounding if precision is less than 120 places.
// ----------------------------------------------------------------------------
int math_const_e(BigDecimal *out, int precision) {
    // Validate output destination pointer
    if (!out) return 0;

    // Negative precision is clamped to 0 decimal places
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed string into BigDecimal structure
    if (!decimal_from_string(out, E_STR_120)) return 0;

    // Round down to requested precision if user asked for fewer than 120 places
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// ----------------------------------------------------------------------------
// Function: math_const_ln10
// What: Generates the constant ln(10) rounded to target precision.
// How:
//   1. Deserializes the precomputed 120-digit ln(10) string.
//   2. Rounds the result to the requested precision using ROUND_HALF_UP.
//   3. Used internally by fn_ln() for base-10 range reduction.
// ----------------------------------------------------------------------------
int math_const_ln10(BigDecimal *out, int precision) {
    // Validate output destination pointer
    if (!out) return 0;

    // Negative precision is clamped to 0 decimal places
    if (precision < 0) precision = 0;

    // Load full 120-digit precomputed string into BigDecimal structure
    if (!decimal_from_string(out, LN10_STR_120)) return 0;

    // Round down to requested precision if user asked for fewer than 120 places
    if (out->scale > precision) {
        decimal_round(out, precision, ROUND_HALF_UP);
    }
    return 1;
}

// ----------------------------------------------------------------------------
// Helper Function: decimal_abs
// What: Computes the absolute value of a BigDecimal (|src|).
// How:
//   1. Copies src to dest if they point to different memory locations.
//   2. Forces the significand sign to positive (+1) if it was negative (-1).
// ----------------------------------------------------------------------------
static int decimal_abs(BigDecimal *dest, const BigDecimal *src) {
    if (!dest || !src) return 0;

    // Deep copy if source and destination are separate structures
    if (dest != src) {
        decimal_free(dest);
        if (!decimal_copy(dest, src)) return 0;
    }

    // Force sign flag to positive
    if (dest->significand.sign < 0) {
        dest->significand.sign = 1;
    }
    return 1;
}

// ----------------------------------------------------------------------------
// Function: fn_sqrt
// What: Computes the square root sqrt(x) to requested decimal precision.
// How:
//   1. Validates mathematical domain: negative numbers are rejected (returns 0).
//   2. Base case: sqrt(0) returns 0 immediately.
//   3. Adds 8 internal guard digits (guard_prec = precision + 8) to eliminate
//      intermediate round-off cancellation errors.
//   4. Obtains a rapid 64-bit hardware floating-point estimate (sqrt(dval))
//      to seed the iteration close to the true root.
//   5. Applies Newton-Raphson iteration:
//        y_{k+1} = 0.5 * (y_k + x / y_k)
//      Each step doubles the number of accurate decimal digits (quadratic convergence).
//   6. Terminates when |y_{k+1} - y_k| <= 10^(-precision) or max 80 iterations reached.
//   7. Rounds final y to target precision with ROUND_HALF_UP and frees all temporaries.
// ----------------------------------------------------------------------------
int fn_sqrt(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Domain check: Square root of negative number is undefined over real numbers
    if (x->significand.sign < 0) {
        return 0;
    }

    // Trivial case: sqrt(0) = 0
    if (decimal_is_zero(x)) {
        bigint_clear(&out->significand);
        out->scale = 0;
        return 1;
    }

    // Guard precision: 8 additional fractional digits protect against rounding drift
    int guard_prec = precision + 8;

    // Step 1: Generate initial seed using hardware double precision
    char *s = decimal_to_string(x);
    if (!s) return 0;
    double dval = atof(s);
    free(s);

    // Initial floating-point guess
    double guess_d = (dval > 0.0) ? sqrt(dval) : 1.0;
    char guess_str[64];
    snprintf(guess_str, sizeof(guess_str), "%.12g", guess_d);

    // Initialize iteration variables
    BigDecimal y, next_y, div_val, sum_val, diff;
    decimal_init(&y);
    decimal_init(&next_y);
    decimal_init(&div_val);
    decimal_init(&sum_val);
    decimal_init(&diff);

    // Load initial guess string into BigDecimal y
    if (!decimal_from_string(&y, guess_str)) {
        decimal_free(&y); decimal_free(&next_y);
        decimal_free(&div_val); decimal_free(&sum_val);
        decimal_free(&diff);
        return 0;
    }

    // Constant 2 used for the Newton-Raphson division by 2
    BigDecimal two;
    decimal_init(&two);
    decimal_from_string(&two, "2");

    // Epsilon threshold: 10^(-precision) represents target convergence tolerance
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = precision;

    // Step 2: Newton-Raphson loop (quadratically converges in <= 80 iterations)
    int max_iters = 80;
    for (int iter = 0; iter < max_iters; iter++) {
        // div_val = x / y_k (computed with guard precision)
        if (!decimal_div(&div_val, x, &y, guard_prec)) break;

        // sum_val = y_k + (x / y_k)
        if (!decimal_add(&sum_val, &y, &div_val)) break;

        // next_y = sum_val / 2
        if (!decimal_div(&next_y, &sum_val, &two, guard_prec)) break;

        // diff = |next_y - y_k| (measures distance between consecutive estimates)
        if (!decimal_sub(&diff, &next_y, &y)) break;
        decimal_abs(&diff, &diff);

        // Advance to next estimate: y_k = next_y
        decimal_copy(&y, &next_y);

        // Check convergence condition: stop when difference <= epsilon
        if (decimal_compare(&diff, &epsilon) <= 0) {
            break;
        }
    }

    // Step 3: Round the converged result to requested precision
    decimal_round(&y, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &y); // Swap result into caller's out buffer in O(1) time

    // Step 4: Deallocate all temporary calculation BigDecimals
    decimal_free(&y);
    decimal_free(&next_y);
    decimal_free(&div_val);
    decimal_free(&sum_val);
    decimal_free(&diff);
    decimal_free(&two);
    decimal_free(&epsilon);
    return 1;
}

// ----------------------------------------------------------------------------
// Function: fn_exp
// What: Computes the exponential function exp(x) = e^x to target decimal precision.
// How:
//   1. Base case: exp(0) = 1.
//   2. Handles negative inputs via reciprocal identity: exp(-x) = 1 / exp(x).
//   3. Range reduction: Splits input x into integer part m and fractional part f:
//        x = m + f, where m is integer and 0 <= f < 1.
//      Then: exp(x) = e^m * exp(f).
//   4. Computes e^m using binary repeated squaring on precomputed constant e.
//      Enforces safety limit m <= 5000 to prevent runaway CPU time.
//   5. Computes exp(f) using Maclaurin series expansion for 0 <= f < 1:
//        exp(f) = sum_{k=0}^N f^k / k! = 1 + f + f^2/2! + f^3/3! + ...
//      Because f < 1, the series converges factorially fast (terminates in < 200 terms).
//   6. Multiplies the two components: res = e^m * exp(f).
//   7. Rounds to target precision with ROUND_HALF_UP and frees all temporaries.
// ----------------------------------------------------------------------------
int fn_exp(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Trivial case: e^0 = 1
    if (decimal_is_zero(x)) {
        decimal_from_string(out, "1");
        decimal_round(out, precision, ROUND_HALF_UP);
        return 1;
    }

    // Guard precision: 12 extra digits isolate intermediate series accumulation errors
    int guard_prec = precision + 12;

    // Step 1: Negative input transformation via reciprocal: exp(-x) = 1 / exp(x)
    if (x->significand.sign < 0) {
        BigDecimal pos_x, exp_pos, one;
        decimal_init(&pos_x);
        decimal_init(&exp_pos);
        decimal_init(&one);

        // Create positive copy: |x|
        decimal_copy(&pos_x, x);
        pos_x.significand.sign = 1;
        decimal_from_string(&one, "1");

        // Compute exp(|x|) recursively
        if (!fn_exp(&exp_pos, &pos_x, guard_prec)) {
            decimal_free(&pos_x); decimal_free(&exp_pos); decimal_free(&one);
            return 0;
        }

        // Divide 1 / exp(|x|) to target precision
        if (!decimal_div(out, &one, &exp_pos, precision)) {
            decimal_free(&pos_x); decimal_free(&exp_pos); decimal_free(&one);
            return 0;
        }
        decimal_round(out, precision, ROUND_HALF_UP);

        decimal_free(&pos_x);
        decimal_free(&exp_pos);
        decimal_free(&one);
        return 1;
    }

    // Step 2: Range reduction - decompose positive x into m (integer) and f (fraction)
    // x = m + f, where 0 <= f < 1
    BigDecimal m_dec, f_dec;
    decimal_init(&m_dec);
    decimal_init(&f_dec);
    decimal_copy(&m_dec, x);
    decimal_round(&m_dec, 0, ROUND_TRUNC); // Truncate fractional digits to obtain integer m
    decimal_sub(&f_dec, x, &m_dec);        // f = x - m (fractional remainder 0 <= f < 1)

    // Step 3: Compute e^m via repeated squaring binary exponentiation
    BigDecimal e_const, e_pow_m;
    decimal_init(&e_const);
    decimal_init(&e_pow_m);
    math_const_e(&e_const, guard_prec); // Load constant e with guard precision
    decimal_from_string(&e_pow_m, "1"); // Initialize accumulator = 1

    char *m_str = decimal_to_string(&m_dec);
    if (m_str) {
        long m_val = atol(m_str);
        free(m_str);

        // Safety limit: Guard against exponential overflow for exponents > 5000
        if (m_val > 5000) {
            decimal_free(&m_dec); decimal_free(&f_dec);
            decimal_free(&e_const); decimal_free(&e_pow_m);
            return 0;
        }

        // Binary exponentiation for base^p: computes e^m in O(log m) multiplications
        BigDecimal base, acc;
        decimal_init(&base); decimal_init(&acc);
        decimal_copy(&base, &e_const);
        decimal_copy(&acc, &e_pow_m);

        long p = m_val;
        while (p > 0) {
            // If current bit is 1, multiply accumulator by current base power
            if (p & 1) {
                decimal_mul(&acc, &acc, &base);
                decimal_round(&acc, guard_prec, ROUND_HALF_UP);
            }
            p >>= 1; // Shift to next bit
            if (p > 0) {
                // Square the base for next bit position: base = base^2
                decimal_mul(&base, &base, &base);
                decimal_round(&base, guard_prec, ROUND_HALF_UP);
            }
        }
        decimal_swap(&e_pow_m, &acc);
        decimal_free(&base);
        decimal_free(&acc);
    }

    // Step 4: Taylor series expansion for exp(f) where 0 <= f < 1
    // exp(f) = sum_{k=0}^N f^k / k! = 1 + f + f^2/2! + f^3/3! + ...
    BigDecimal sum, term, k_dec, tmp_term;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&k_dec);
    decimal_init(&tmp_term);

    // Initial term k=0 is 1: sum = 1, term = 1
    decimal_from_string(&sum, "1");
    decimal_from_string(&term, "1");

    // Epsilon convergence threshold: 10^(-guard_prec)
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    // Series iteration loop (converges rapidly due to k! factorial denominator)
    for (int k = 1; k < 200; k++) {
        // Compute next term iteratively: term_k = term_{k-1} * f / k
        decimal_mul(&tmp_term, &term, &f_dec);
        char k_buf[32];
        snprintf(k_buf, sizeof(k_buf), "%d", k);
        decimal_from_string(&k_dec, k_buf);
        decimal_div(&term, &tmp_term, &k_dec, guard_prec);

        // Accumulate term into total sum: sum += term_k
        decimal_add(&sum, &sum, &term);

        // Check if term has decayed below epsilon convergence threshold
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break; // Reached required precision
    }

    // Step 5: Combine integer and fractional components: res = e^m * exp(f)
    BigDecimal res;
    decimal_init(&res);
    decimal_mul(&res, &e_pow_m, &sum);
    decimal_round(&res, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &res); // Transfer result to caller output buffer

    // Step 6: Free all allocated temporary structures
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

// ----------------------------------------------------------------------------
// Function: fn_ln
// What: Computes the natural logarithm ln(x) for x > 0 to target decimal precision.
// How:
//   1. Validates mathematical domain: ln(x) is undefined for x <= 0 (returns 0).
//   2. Adds 12 guard digits (guard_prec = precision + 12).
//   3. Range reduction via power of 10 scaling:
//        x = y * 10^k, such that 0.5 <= y < 5.0.
//      Then by logarithmic identity:
//        ln(x) = k * ln(10) + ln(y).
//   4. High-speed series substitution: Let u = (y - 1) / (y + 1).
//      Then ln(y) is computed via the rapidly converging area hyperbolic series:
//        ln(y) = 2 * sum_{i=0}^N (1 / (2i + 1)) * u^(2i + 1)
//              = 2 * [ u + u^3/3 + u^5/5 + u^7/7 + ... ]
//      Because |u| < 0.67, powers u^(2i+1) decay exponentially fast.
//   5. Multiplies sum by 2, adds k * ln(10), and rounds to target precision.
//   6. Deallocates all temporary structures.
// ----------------------------------------------------------------------------
int fn_ln(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Domain validation: ln(x) is mathematically undefined for x <= 0
    if (x->significand.sign <= 0 || decimal_is_zero(x)) {
        return 0;
    }

    // Guard precision: 12 extra fractional digits to prevent truncation drift
    int guard_prec = precision + 12;

    // Step 1: Estimate power-of-10 exponent k to scale x into range [0.5, 5.0)
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

    // Scale x by 10^k: y = x / 10^k
    if (k_pow10 >= 0) {
        bigint_set_u64(&scale_factor.significand, 1);
        bigint_mul_pow10(&scale_factor.significand, &scale_factor.significand, (size_t)k_pow10);
        scale_factor.scale = 0;
        decimal_div(&y, x, &scale_factor, guard_prec);
    } else {
        bigint_set_u64(&scale_factor.significand, 1);
        bigint_mul_pow10(&scale_factor.significand, &scale_factor.significand, (size_t)(-k_pow10));
        scale_factor.scale = 0;
        decimal_mul(&y, x, &scale_factor);
        decimal_round(&y, guard_prec, ROUND_HALF_UP);
    }

    // Step 2: Compute substituted variable u = (y - 1) / (y + 1)
    BigDecimal u, y_minus_1, y_plus_1;
    decimal_init(&u);
    decimal_init(&y_minus_1);
    decimal_init(&y_plus_1);

    decimal_sub(&y_minus_1, &y, &one);             // numerator = y - 1
    decimal_add(&y_plus_1, &y, &one);              // denominator = y + 1
    decimal_div(&u, &y_minus_1, &y_plus_1, guard_prec); // u = (y - 1) / (y + 1)

    // Compute u2 = u^2 for stepping through odd powers: u^(2i+1) = u^(2i-1) * u^2
    BigDecimal u2, u_pow;
    decimal_init(&u2);
    decimal_init(&u_pow);
    decimal_mul(&u2, &u, &u);
    decimal_round(&u2, guard_prec, ROUND_HALF_UP);
    decimal_copy(&u_pow, &u); // Initial power u^1 = u

    // Step 3: Accumulate series: sum_{i=0}^N (1 / (2i + 1)) * u^(2i + 1)
    BigDecimal series_sum, term, denom_dec;
    decimal_init(&series_sum);
    decimal_init(&term);
    decimal_init(&denom_dec);

    // Epsilon threshold: 10^(-guard_prec)
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    for (int i = 0; i < 200; i++) {
        // Odd denominator: 2*i + 1 (1, 3, 5, 7, ...)
        int denom = 2 * i + 1;
        char denom_str[32];
        snprintf(denom_str, sizeof(denom_str), "%d", denom);
        decimal_from_string(&denom_dec, denom_str);

        // term = u^(2i+1) / (2i+1)
        decimal_div(&term, &u_pow, &denom_dec, guard_prec);
        decimal_add(&series_sum, &series_sum, &term);

        // Check convergence against epsilon tolerance
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;

        // Multiply by u^2 to prepare next odd power: u_pow *= u^2
        decimal_mul(&u_pow, &u_pow, &u2);
        decimal_round(&u_pow, guard_prec, ROUND_HALF_UP);
    }

    // Step 4: Scale series sum by 2: ln(y) = 2 * series_sum
    BigDecimal ln_y, two;
    decimal_init(&ln_y);
    decimal_init(&two);
    decimal_from_string(&two, "2");
    decimal_mul(&ln_y, &two, &series_sum);
    decimal_round(&ln_y, guard_prec, ROUND_HALF_UP);

    // Step 5: Compute power-of-10 component: k * ln(10)
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

    // Step 6: Total logarithm = ln(y) + k * ln(10)
    BigDecimal total;
    decimal_init(&total);
    decimal_add(&total, &ln_y, &k_ln10);
    decimal_round(&total, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &total);

    // Step 7: Deallocate temporary structures
    decimal_free(&y); decimal_free(&scale_factor); decimal_free(&one);
    decimal_free(&u); decimal_free(&y_minus_1); decimal_free(&y_plus_1);
    decimal_free(&u2); decimal_free(&u_pow);
    decimal_free(&series_sum); decimal_free(&term); decimal_free(&denom_dec);
    decimal_free(&epsilon); decimal_free(&ln_y); decimal_free(&two);
    decimal_free(&k_ln10); decimal_free(&ln10); decimal_free(&k_dec);
    decimal_free(&total);
    return 1;
}

// ----------------------------------------------------------------------------
// Helper Function: reduce_angle_mod_2pi
// What: Reduces an arbitrary angle x modulo 2*pi into the primary interval [-pi, pi].
// How:
//   1. Computes high-precision 2*pi = 2 * pi with guard precision.
//   2. Computes the nearest integer cycle count: k = round(x / (2*pi)).
//   3. Subtracts the full cycles: x_red = x - k * (2*pi).
//   4. This restricts x_red to [-pi, pi], ensuring fast Taylor series convergence.
// ----------------------------------------------------------------------------
static int reduce_angle_mod_2pi(BigDecimal *x_red, const BigDecimal *x, int guard_prec) {
    BigDecimal pi_val, two_pi;
    decimal_init(&pi_val);
    decimal_init(&two_pi);

    // Load constant pi with guard precision and multiply by 2
    math_const_pi(&pi_val, guard_prec);
    BigDecimal two;
    decimal_init(&two);
    decimal_from_string(&two, "2");
    decimal_mul(&two_pi, &pi_val, &two);
    decimal_round(&two_pi, guard_prec, ROUND_HALF_UP);

    // Calculate nearest integer multiple of 2*pi: k = round(x / 2pi)
    BigDecimal div_res, k_round, k_two_pi;
    decimal_init(&div_res);
    decimal_init(&k_round);
    decimal_init(&k_two_pi);

    decimal_div(&div_res, x, &two_pi, guard_prec);
    decimal_copy(&k_round, &div_res);
    decimal_round(&k_round, 0, ROUND_HALF_UP); // Round to nearest integer

    // Compute x_red = x - k * 2pi
    decimal_mul(&k_two_pi, &k_round, &two_pi);
    decimal_sub(x_red, x, &k_two_pi);
    decimal_round(x_red, guard_prec, ROUND_HALF_UP);

    // Free temporary structures
    decimal_free(&pi_val);
    decimal_free(&two_pi);
    decimal_free(&two);
    decimal_free(&div_res);
    decimal_free(&k_round);
    decimal_free(&k_two_pi);
    return 1;
}

// ----------------------------------------------------------------------------
// Function: fn_sin
// What: Computes the trigonometric sine sin(x) to requested decimal precision.
// How:
//   1. Base case: sin(0) = 0.
//   2. Adds 12 guard digits to protect precision during division.
//   3. Applies range reduction: x_red = x mod 2*pi so x_red is in [-pi, pi].
//   4. Computes truncated alternating Maclaurin series:
//        sin(x) = sum_{k=0}^N (-1)^k * x^(2k+1) / (2k+1)!
//               = x - x^3/3! + x^5/5! - x^7/7! + ...
//   5. Iteratively updates terms:
//        term_k = -term_{k-1} * x^2 / ((2k) * (2k+1))
//   6. Terminates when |term_k| <= 10^(-guard_prec) or 150 terms reached.
//   7. Rounds to target precision with ROUND_HALF_UP and frees all temporaries.
// ----------------------------------------------------------------------------
int fn_sin(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Trivial case: sin(0) = 0
    if (decimal_is_zero(x)) {
        bigint_clear(&out->significand);
        out->scale = 0;
        return 1;
    }

    // Guard precision: 12 extra digits isolate intermediate truncation noise
    int guard_prec = precision + 12;

    // Step 1: Range reduction - map angle modulo 2*pi into [-pi, pi]
    BigDecimal x_red;
    decimal_init(&x_red);
    reduce_angle_mod_2pi(&x_red, x, guard_prec);

    // Precompute x2 = x_red^2 to reuse across series iterations
    BigDecimal x2;
    decimal_init(&x2);
    decimal_mul(&x2, &x_red, &x_red);
    decimal_round(&x2, guard_prec, ROUND_HALF_UP);

    // Step 2: Initialize Maclaurin series with term k=0: sum = x_red, term = x_red
    BigDecimal sum, term, tmp_term, denom_dec;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&tmp_term);
    decimal_init(&denom_dec);

    decimal_copy(&sum, &x_red);
    decimal_copy(&term, &x_red);

    // Epsilon threshold: 10^(-guard_prec)
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    // Step 3: Alternating series expansion loop
    for (int k = 1; k < 150; k++) {
        // Compute numerator: tmp_term = term * x^2
        decimal_mul(&tmp_term, &term, &x2);

        // Denominator step factor: (2k) * (2k + 1)
        long long denom = (long long)(2 * k) * (long long)(2 * k + 1);
        char d_buf[32];
        snprintf(d_buf, sizeof(d_buf), "%lld", denom);
        decimal_from_string(&denom_dec, d_buf);

        // Divide: term = tmp_term / ((2k)*(2k+1))
        decimal_div(&term, &tmp_term, &denom_dec, guard_prec);
        term.significand.sign = -term.significand.sign; // Alternating sign: (-1)^k factor

        // Accumulate into total sum: sum += term
        decimal_add(&sum, &sum, &term);

        // Check convergence against epsilon tolerance
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;
    }

    // Step 4: Round final sum to target precision and swap to output
    decimal_round(&sum, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &sum);

    // Step 5: Deallocate temporary BigDecimals
    decimal_free(&x_red);
    decimal_free(&x2);
    decimal_free(&sum);
    decimal_free(&term);
    decimal_free(&tmp_term);
    decimal_free(&denom_dec);
    decimal_free(&epsilon);
    return 1;
}

// ----------------------------------------------------------------------------
// Function: fn_cos
// What: Computes the trigonometric cosine cos(x) to requested decimal precision.
// How:
//   1. Base case: cos(0) = 1.
//   2. Adds 12 guard digits to protect precision during division.
//   3. Applies range reduction: x_red = x mod 2*pi so x_red is in [-pi, pi].
//   4. Computes truncated alternating Maclaurin series:
//        cos(x) = sum_{k=0}^N (-1)^k * x^(2k) / (2k)!
//               = 1 - x^2/2! + x^4/4! - x^6/6! + ...
//   5. Iteratively updates terms:
//        term_k = -term_{k-1} * x^2 / ((2k-1) * (2k))
//   6. Terminates when |term_k| <= 10^(-guard_prec) or 150 terms reached.
//   7. Rounds to target precision with ROUND_HALF_UP and frees all temporaries.
// ----------------------------------------------------------------------------
int fn_cos(BigDecimal *out, const BigDecimal *x, int precision) {
    if (!out || !x) return 0;
    if (precision < 0) precision = 0;

    // Trivial case: cos(0) = 1
    if (decimal_is_zero(x)) {
        decimal_from_string(out, "1");
        decimal_round(out, precision, ROUND_HALF_UP);
        return 1;
    }

    // Guard precision: 12 extra digits isolate intermediate truncation noise
    int guard_prec = precision + 12;

    // Step 1: Range reduction - map angle modulo 2*pi into [-pi, pi]
    BigDecimal x_red;
    decimal_init(&x_red);
    reduce_angle_mod_2pi(&x_red, x, guard_prec);

    // Precompute x2 = x_red^2 to reuse across series iterations
    BigDecimal x2;
    decimal_init(&x2);
    decimal_mul(&x2, &x_red, &x_red);
    decimal_round(&x2, guard_prec, ROUND_HALF_UP);

    // Step 2: Initialize Maclaurin series with term k=0: sum = 1, term = 1
    BigDecimal sum, term, tmp_term, denom_dec;
    decimal_init(&sum);
    decimal_init(&term);
    decimal_init(&tmp_term);
    decimal_init(&denom_dec);

    decimal_from_string(&sum, "1");
    decimal_from_string(&term, "1");

    // Epsilon threshold: 10^(-guard_prec)
    BigDecimal epsilon;
    decimal_init(&epsilon);
    bigint_set_u64(&epsilon.significand, 1);
    epsilon.scale = guard_prec;

    // Step 3: Alternating series expansion loop
    for (int k = 1; k < 150; k++) {
        // Compute numerator: tmp_term = term * x^2
        decimal_mul(&tmp_term, &term, &x2);

        // Denominator step factor: (2k - 1) * (2k)
        long long denom = (long long)(2 * k - 1) * (long long)(2 * k);
        char d_buf[32];
        snprintf(d_buf, sizeof(d_buf), "%lld", denom);
        decimal_from_string(&denom_dec, d_buf);

        // Divide: term = tmp_term / ((2k-1)*(2k))
        decimal_div(&term, &tmp_term, &denom_dec, guard_prec);
        term.significand.sign = -term.significand.sign; // Alternating sign: (-1)^k factor

        // Accumulate into total sum: sum += term
        decimal_add(&sum, &sum, &term);

        // Check convergence against epsilon tolerance
        BigDecimal abs_term;
        decimal_init(&abs_term);
        decimal_abs(&abs_term, &term);
        int done = (decimal_compare(&abs_term, &epsilon) <= 0);
        decimal_free(&abs_term);
        if (done) break;
    }

    // Step 4: Round final sum to target precision and swap to output
    decimal_round(&sum, precision, ROUND_HALF_UP);
    decimal_free(out);
    decimal_swap(out, &sum);

    // Step 5: Deallocate temporary BigDecimals
    decimal_free(&x_red);
    decimal_free(&x2);
    decimal_free(&sum);
    decimal_free(&term);
    decimal_free(&tmp_term);
    decimal_free(&denom_dec);
    decimal_free(&epsilon);
    return 1;
}
