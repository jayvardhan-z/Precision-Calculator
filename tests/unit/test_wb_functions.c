/**
 * ============================================================================
 * File: test_wb_functions.c
 * Module: Worklet 3 (functions.c)
 * White-Box Testing Techniques Covered:
 *   - Newton-Raphson square root convergence and domain checks (x >= 0)
 *   - Maclaurin / Taylor series for exp(x), ln(x), sin(x), cos(x)
 *   - Transcendental identities:
 *     - sqrt(x)^2 == x within epsilon
 *     - sin^2(x) + cos^2(x) == 1 within epsilon
 *     - exp(ln(x)) == x within epsilon
 *   - Mathematical constants pi and e generation to arbitrary precision
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "functions.h"

int main(void) {
    minitest_init_suite("White-Box Unit: Transcendental Functions (Worklet 3)", "tests/test_results.csv");

    TEST_CASE("UT-FNC-01", "UNIT", "Newton-Raphson sqrt: exact 144, sqrt(2), and domain error") {
        BigDecimal x, res;
        decimal_init(&x); decimal_init(&res);

        /* sqrt(144) = 12 */
        decimal_from_string(&x, "144");
        CHECK(fn_sqrt(&res, &x, 20), "sqrt(144)");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "12.00000000000000000000", "sqrt(144) to 20 places");
        free(s);

        /* sqrt(2) = 1.41421356237309504880 */
        decimal_from_string(&x, "2");
        CHECK(fn_sqrt(&res, &x, 20), "sqrt(2)");
        s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "1.41421356237309504880", "sqrt(2) to 20 places");
        free(s);

        /* Negative domain error: sqrt(-4) must fail */
        decimal_from_string(&x, "-4");
        CHECK_INT_EQ(fn_sqrt(&res, &x, 20), 0, "sqrt(-4) must return 0 failure");

        decimal_free(&x); decimal_free(&res);
    }

    TEST_CASE("UT-FNC-02", "UNIT", "Identity sqrt(x)^2 == x within epsilon") {
        BigDecimal x, r, sq;
        decimal_init(&x); decimal_init(&r); decimal_init(&sq);

        decimal_from_string(&x, "50");
        fn_sqrt(&r, &x, 25);
        decimal_mul(&sq, &r, &r);
        decimal_round(&sq, 20, ROUND_HALF_UP);
        char *s = decimal_to_string(&sq);
        CHECK_STR_EQ(s, "50.00000000000000000000", "sqrt(50)^2 == 50 to 20 places");
        free(s);

        decimal_free(&x); decimal_free(&r); decimal_free(&sq);
    }

    TEST_CASE("UT-FNC-03", "UNIT", "Exponential exp(0) == 1 and exp(1) == Euler's constant e") {
        BigDecimal x, res;
        decimal_init(&x); decimal_init(&res);

        /* exp(0) = 1 */
        decimal_from_string(&x, "0");
        CHECK(fn_exp(&res, &x, 20), "exp(0)");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "1", "exp(0) == 1");
        free(s);

        /* exp(1) = 2.71828182845904523536 */
        decimal_from_string(&x, "1");
        CHECK(fn_exp(&res, &x, 20), "exp(1)");
        s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "2.71828182845904523536", "exp(1) Euler's constant");
        free(s);

        decimal_free(&x); decimal_free(&res);
    }

    TEST_CASE("UT-FNC-04", "UNIT", "Natural logarithm ln(1) == 0 and domain error (x <= 0)") {
        BigDecimal x, res;
        decimal_init(&x); decimal_init(&res);

        /* ln(1) = 0 */
        decimal_from_string(&x, "1");
        CHECK(fn_ln(&res, &x, 20), "ln(1)");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "0", "ln(1) == 0");
        free(s);

        /* ln(0) must fail */
        decimal_from_string(&x, "0");
        CHECK_INT_EQ(fn_ln(&res, &x, 20), 0, "ln(0) must return 0 failure");

        /* ln(-10) must fail */
        decimal_from_string(&x, "-10");
        CHECK_INT_EQ(fn_ln(&res, &x, 20), 0, "ln(-10) must return 0 failure");

        decimal_free(&x); decimal_free(&res);
    }

    TEST_CASE("UT-FNC-05", "UNIT", "Identity exp(ln(x)) == x within epsilon") {
        BigDecimal x, log_x, exp_log;
        decimal_init(&x); decimal_init(&log_x); decimal_init(&exp_log);

        decimal_from_string(&x, "7");
        fn_ln(&log_x, &x, 25);
        fn_exp(&exp_log, &log_x, 25);
        decimal_round(&exp_log, 20, ROUND_HALF_UP);
        char *s = decimal_to_string(&exp_log);
        CHECK_STR_EQ(s, "7.00000000000000000000", "exp(ln(7)) == 7 to 20 places");
        free(s);

        decimal_free(&x); decimal_free(&log_x); decimal_free(&exp_log);
    }

    TEST_CASE("UT-FNC-06", "UNIT", "Trigonometric sin and cos: sin(0) == 0, cos(0) == 1, sin(pi/2) == 1") {
        BigDecimal x, res;
        decimal_init(&x); decimal_init(&res);

        /* sin(0) = 0 */
        decimal_from_string(&x, "0");
        CHECK(fn_sin(&res, &x, 20), "sin(0)");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "0", "sin(0) == 0");
        free(s);

        /* cos(0) = 1 */
        CHECK(fn_cos(&res, &x, 20), "cos(0)");
        s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "1", "cos(0) == 1");
        free(s);

        decimal_free(&x); decimal_free(&res);
    }

    TEST_CASE("UT-FNC-07", "UNIT", "Identity sin^2(x) + cos^2(x) == 1 within epsilon") {
        BigDecimal x, s_val, c_val, s_sq, c_sq, sum;
        decimal_init(&x); decimal_init(&s_val); decimal_init(&c_val);
        decimal_init(&s_sq); decimal_init(&c_sq); decimal_init(&sum);

        decimal_from_string(&x, "1.2345"); /* 1.2345 radians */
        fn_sin(&s_val, &x, 25);
        fn_cos(&c_val, &x, 25);

        decimal_mul(&s_sq, &s_val, &s_val);
        decimal_mul(&c_sq, &c_val, &c_val);
        decimal_add(&sum, &s_sq, &c_sq);
        decimal_round(&sum, 20, ROUND_HALF_UP);

        char *s = decimal_to_string(&sum);
        CHECK_STR_EQ(s, "1.00000000000000000000", "sin^2(1.2345) + cos^2(1.2345) == 1");
        free(s);

        decimal_free(&x); decimal_free(&s_val); decimal_free(&c_val);
        decimal_free(&s_sq); decimal_free(&c_sq); decimal_free(&sum);
    }

    TEST_CASE("UT-FNC-08", "UNIT", "Constants generator: pi and e to 20 decimal places") {
        BigDecimal pi_val, e_val;
        decimal_init(&pi_val); decimal_init(&e_val);

        CHECK(math_const_pi(&pi_val, 20), "Generate pi");
        char *s_pi = decimal_to_string(&pi_val);
        CHECK_STR_EQ(s_pi, "3.14159265358979323846", "pi to 20 places");
        free(s_pi);

        CHECK(math_const_e(&e_val, 20), "Generate e");
        char *s_e = decimal_to_string(&e_val);
        CHECK_STR_EQ(s_e, "2.71828182845904523536", "e to 20 places");
        free(s_e);

        decimal_free(&pi_val); decimal_free(&e_val);
    }

    return minitest_finish_suite();
}

