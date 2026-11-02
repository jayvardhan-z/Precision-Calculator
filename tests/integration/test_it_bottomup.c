/**
 * ============================================================================
 * File: test_it_bottomup.c
 * Module: Integration Testing - Bottom-Up Approach
 * Description:
 *   Integrates fundamental mathematical modules starting from the lowest level:
 *   BigInteger -> BigDecimal -> Complex.
 *   Uses drivers to verify data integrity across layer boundaries:
 *     - BigInteger addition/multiplication/division driving BigDecimal scale shifts
 *     - BigDecimal operations composing Complex number real/imag arithmetic
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "bigint.h"
#include "bigint_io.h"
#include "decimal.h"
#include "complex.h"
#include "fast_math.h"

// External driver routines from driver_arithmetic.c
extern int driver_run_bigint_pipeline(const char *s_a, const char *op, const char *s_b, char **out_str);
extern int driver_run_decimal_pipeline(const char *s_a, const char *s_b, int precision, char **out_str);

int main(void) {
    minitest_init_suite("Integration: Incremental Bottom-Up (Drivers)", "tests/test_results.csv");

    TEST_CASE("IT-BU-01", "INTEG", "Driver executes BigInteger addition across limb boundaries") {
        char *res_str = NULL;
        /* 999999999 + 1 = 1000000000 (triggers multi-limb expansion) */
        CHECK(driver_run_bigint_pipeline("999999999", "+", "1", &res_str), "Driver runs addition");
        CHECK_NOT_NULL(res_str, "Output string allocated");
        CHECK_STR_EQ(res_str, "1000000000", "Result matches exact 10^9 boundary");
        free(res_str);
    }

    TEST_CASE("IT-BU-02", "INTEG", "Driver executes Knuth D division through driver pipeline") {
        char *res_str = NULL;
        CHECK(driver_run_bigint_pipeline("1000000000000", "/", "1000000", &res_str), "Driver runs Knuth D");
        CHECK_NOT_NULL(res_str, "Output string allocated");
        CHECK_STR_EQ(res_str, "1000000", "10^12 / 10^6 == 10^6");
        free(res_str);
    }

    TEST_CASE("IT-BU-03", "INTEG", "Driver integrates BigInteger into BigDecimal division") {
        char *res_str = NULL;
        CHECK(driver_run_decimal_pipeline("1", "8", 5, &res_str), "Driver runs decimal division");
        CHECK_NOT_NULL(res_str, "Output string allocated");
        CHECK_STR_EQ(res_str, "0.12500", "1 / 8 with precision 5 == 0.12500");
        free(res_str);
    }

    TEST_CASE("IT-BU-04", "INTEG", "BigDecimal layer successfully composes Complex arithmetic") {
        Complex a, b, res;
        complex_init(&a);
        complex_init(&b);
        complex_init(&res);

        /* a = 3 + 4i, b = 1 - 2i */
        decimal_from_string(&a.real, "3");
        decimal_from_string(&a.imag, "4");
        decimal_from_string(&b.real, "1");
        decimal_from_string(&b.imag, "-2");

        /* (3 + 4i) * (1 - 2i) = (3*1 - 4*(-2)) + (3*(-2) + 4*1)i = (3 + 8) + (-6 + 4)i = 11 - 2i */
        CHECK(complex_mul(&res, &a, &b), "complex_mul executes");

        char *s_real = decimal_to_string(&res.real);
        char *s_imag = decimal_to_string(&res.imag);

        CHECK_STR_EQ(s_real, "11", "Real part is 11");
        CHECK_STR_EQ(s_imag, "-2", "Imag part is -2");

        free(s_real);
        free(s_imag);
        complex_free(&a);
        complex_free(&b);
        complex_free(&res);
    }

    return minitest_finish_suite();
}

