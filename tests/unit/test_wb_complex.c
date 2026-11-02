/**
 * ============================================================================
 * File: test_wb_complex.c
 * Module: Worklet 3 (complex.c)
 * White-Box Testing Techniques Covered:
 *   - Component-wise complex addition and subtraction
 *   - Complex multiplication: fundamental imaginary unit identity i^2 = -1
 *   - Conjugate multiplication: (a + bi)(a - bi) = a^2 + b^2
 *   - Complex division via conjugate scaling
 *   - String rendering: pure real ("5"), pure imaginary ("3i"), full ("2 + 3i", "2 - 3i")
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "complex.h"

int main(void) {
    minitest_init_suite("White-Box Unit: Complex Numbers (Worklet 3)", "tests/test_results.csv");

    TEST_CASE("UT-CPX-01", "UNIT", "Fundamental identity: i * i == -1") {
        Complex i_unit, res;
        complex_init(&i_unit); complex_init(&res);
        complex_set_str(&i_unit, "0", "1"); /* 0 + 1i */

        CHECK(complex_mul(&res, &i_unit, &i_unit), "i * i multiplication");
        char *s = complex_to_string(&res);
        CHECK_STR_EQ(s, "-1", "i * i must equal -1");
        free(s);

        complex_free(&i_unit); complex_free(&res);
    }

    TEST_CASE("UT-CPX-02", "UNIT", "Component-wise addition and subtraction") {
        Complex a, b, res;
        complex_init(&a); complex_init(&b); complex_init(&res);
        complex_set_str(&a, "2", "3");   /* 2 + 3i */
        complex_set_str(&b, "4", "-5");  /* 4 - 5i */

        /* (2 + 3i) + (4 - 5i) = 6 - 2i */
        CHECK(complex_add(&res, &a, &b), "Complex addition");
        char *s = complex_to_string(&res);
        CHECK_STR_EQ(s, "6 - 2i", "(2 + 3i) + (4 - 5i) = 6 - 2i");
        free(s);

        /* (2 + 3i) - (4 - 5i) = -2 + 8i */
        CHECK(complex_sub(&res, &a, &b), "Complex subtraction");
        s = complex_to_string(&res);
        CHECK_STR_EQ(s, "-2 + 8i", "(2 + 3i) - (4 - 5i) = -2 + 8i");
        free(s);

        complex_free(&a); complex_free(&b); complex_free(&res);
    }

    TEST_CASE("UT-CPX-03", "UNIT", "Conjugate multiplication: (2 + 3i) * (2 - 3i) == 13") {
        Complex a, b, res;
        complex_init(&a); complex_init(&b); complex_init(&res);
        complex_set_str(&a, "2", "3");   /* 2 + 3i */
        complex_set_str(&b, "2", "-3");  /* 2 - 3i */

        CHECK(complex_mul(&res, &a, &b), "Conjugate multiplication");
        char *s = complex_to_string(&res);
        CHECK_STR_EQ(s, "13", "(2 + 3i) * (2 - 3i) = 13 (pure real)");
        free(s);

        complex_free(&a); complex_free(&b); complex_free(&res);
    }

    TEST_CASE("UT-CPX-04", "UNIT", "Conjugate division: (1 + i) / (1 - i) == 1i") {
        Complex a, b, res;
        complex_init(&a); complex_init(&b); complex_init(&res);
        complex_set_str(&a, "1", "1");   /* 1 + 1i */
        complex_set_str(&b, "1", "-1");  /* 1 - 1i */

        CHECK(complex_div(&res, &a, &b, 20), "Conjugate division");
        char *s = complex_to_string(&res);
        CHECK_STR_EQ(s, "1.00000000000000000000i", "(1 + i) / (1 - i) = 1i");
        free(s);

        complex_free(&a); complex_free(&b); complex_free(&res);
    }

    TEST_CASE("UT-CPX-05", "UNIT", "Division by complex zero guard returns 0") {
        Complex a, zero, res;
        complex_init(&a); complex_init(&zero); complex_init(&res);
        complex_set_str(&a, "1", "2");
        complex_set_str(&zero, "0", "0");

        CHECK_INT_EQ(complex_div(&res, &a, &zero, 20), 0,
                     "Complex division by 0 + 0i must return 0 failure");

        complex_free(&a); complex_free(&zero); complex_free(&res);
    }

    return minitest_finish_suite();
}

