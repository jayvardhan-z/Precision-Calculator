/**
 * ============================================================================
 * File: stub_functions.c
 * Purpose: Test stub for transcendental and scientific functions (Worklet 3).
 * Role:
 *   Simulates the functions.c module during Top-Down integration testing.
 *   Enables isolated testing of parser and evaluator interfaces without
 *   invoking expensive Taylor series or Newton-Raphson numerical routines.
 *   Provides mock instrumentation (call counts, forced error flags).
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "functions.h"

// Instrumentation counters and controls
int g_stub_sin_calls = 0;
int g_stub_cos_calls = 0;
int g_stub_sqrt_calls = 0;
int g_stub_force_error = 0;

void stub_functions_reset(void) {
    g_stub_sin_calls = 0;
    g_stub_cos_calls = 0;
    g_stub_sqrt_calls = 0;
    g_stub_force_error = 0;
}

int math_const_pi(BigDecimal *out, int precision) {
    (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "3.14159265358979323846");
}

int math_const_e(BigDecimal *out, int precision) {
    (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "2.71828182845904523536");
}

int math_const_ln10(BigDecimal *out, int precision) {
    (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "2.30258509299404568402");
}

int fn_sqrt(BigDecimal *out, const BigDecimal *x, int precision) {
    g_stub_sqrt_calls++;
    if (g_stub_force_error) return 0;
    (void)precision;
    decimal_init(out);
    /* If input is 4, return 2; else return 1.414 */
    if (x->significand.size > 0 && x->significand.digits[0] == 4 && x->scale == 0) {
        return decimal_from_string(out, "2");
    }
    return decimal_from_string(out, "1.4142135623");
}

int fn_exp(BigDecimal *out, const BigDecimal *x, int precision) {
    if (g_stub_force_error) return 0;
    (void)x; (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "2.7182818284");
}

int fn_ln(BigDecimal *out, const BigDecimal *x, int precision) {
    if (g_stub_force_error) return 0;
    (void)x; (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "0");
}

int fn_sin(BigDecimal *out, const BigDecimal *x, int precision) {
    g_stub_sin_calls++;
    if (g_stub_force_error) return 0;
    (void)x; (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "0.5");
}

int fn_cos(BigDecimal *out, const BigDecimal *x, int precision) {
    g_stub_cos_calls++;
    if (g_stub_force_error) return 0;
    (void)x; (void)precision;
    decimal_init(out);
    return decimal_from_string(out, "0.8660254037");
}

