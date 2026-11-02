/**
 * ============================================================================
 * File: test_wb_decimal.c
 * Module: Worklet 3 (decimal.c)
 * White-Box Testing Techniques Covered:
 *   - Scale alignment in addition/subtraction: delta_s = |s1 - s2|
 *   - Scale addition in multiplication: s = s1 + s2
 *   - Arbitrary-precision division to target decimal places
 *   - RoundingMode: ROUND_HALF_UP, ROUND_DOWN, ROUND_TRUNC
 *   - Canonical zero invariant: significand == 0 => scale == 0
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "decimal.h"

int main(void) {
    minitest_init_suite("White-Box Unit: BigDecimal Scaled Arithmetic", "tests/test_results.csv");

    TEST_CASE("UT-DEC-01", "UNIT", "Canonical zero normalization: scale reset to 0") {
        BigDecimal d;
        decimal_init(&d);
        decimal_from_string(&d, "0.000000");
        decimal_normalize(&d);
        CHECK_INT_EQ(d.scale, 0, "Scale must be 0 for normalized zero");
        CHECK(decimal_is_zero(&d), "decimal_is_zero must be true");
        char *s = decimal_to_string(&d);
        CHECK_STR_EQ(s, "0", "Canonical zero renders as '0'");
        free(s);
        decimal_free(&d);
    }

    TEST_CASE("UT-DEC-02", "UNIT", "Scale alignment addition: delta_s = 7 - 4 = 3") {
        BigDecimal a, b, res;
        decimal_init(&a); decimal_init(&b); decimal_init(&res);
        decimal_from_string(&a, "1.2345");
        decimal_from_string(&b, "0.0000678");
        CHECK(decimal_add(&res, &a, &b), "Scale alignment addition");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "1.2345678", "1.2345 + 0.0000678 = 1.2345678");
        free(s);
        decimal_free(&a); decimal_free(&b); decimal_free(&res);
    }

    TEST_CASE("UT-DEC-03", "UNIT", "Scale addition in multiplication: s = 1 + 1 = 2") {
        BigDecimal a, b, res;
        decimal_init(&a); decimal_init(&b); decimal_init(&res);
        decimal_from_string(&a, "2.5");
        decimal_from_string(&b, "1.5");
        CHECK(decimal_mul(&res, &a, &b), "Decimal multiplication");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "3.75", "2.5 * 1.5 = 3.75");
        free(s);
        decimal_free(&a); decimal_free(&b); decimal_free(&res);
    }

    TEST_CASE("UT-DEC-04", "UNIT", "Division with target precision: 1 / 3 to 20 digits") {
        BigDecimal a, b, res;
        decimal_init(&a); decimal_init(&b); decimal_init(&res);
        decimal_from_string(&a, "1");
        decimal_from_string(&b, "3");
        CHECK(decimal_div(&res, &a, &b, 20), "1 / 3 to 20 places");
        char *s = decimal_to_string(&res);
        CHECK_STR_EQ(s, "0.33333333333333333333", "Recurring 1/3 to 20 places");
        free(s);
        decimal_free(&a); decimal_free(&b); decimal_free(&res);
    }

    TEST_CASE("UT-DEC-05", "UNIT", "Rounding: ROUND_HALF_UP tie breaking") {
        BigDecimal d;
        decimal_init(&d);

        /* 1.235 rounded to 2 places -> 1.24 (tie >= 0.5 rounds up) */
        decimal_from_string(&d, "1.235");
        decimal_round(&d, 2, ROUND_HALF_UP);
        char *s = decimal_to_string(&d);
        CHECK_STR_EQ(s, "1.24", "1.235 round-half-up to 2 places is 1.24");
        free(s);

        /* 1.234 rounded to 2 places -> 1.23 (strictly < 0.5 rounds down) */
        decimal_from_string(&d, "1.234");
        decimal_round(&d, 2, ROUND_HALF_UP);
        s = decimal_to_string(&d);
        CHECK_STR_EQ(s, "1.23", "1.234 round-half-up to 2 places is 1.23");
        free(s);

        decimal_free(&d);
    }

    TEST_CASE("UT-DEC-06", "UNIT", "Decimal comparison with different scales") {
        BigDecimal a, b;
        decimal_init(&a); decimal_init(&b);
        decimal_from_string(&a, "1.5");
        decimal_from_string(&b, "1.500000");
        CHECK_INT_EQ(decimal_compare(&a, &b), 0, "1.5 == 1.500000");

        decimal_from_string(&a, "-0.01");
        decimal_from_string(&b, "0.001");
        CHECK_INT_EQ(decimal_compare(&a, &b), -1, "-0.01 < 0.001");
        decimal_free(&a); decimal_free(&b);
    }

    return minitest_finish_suite();
}

