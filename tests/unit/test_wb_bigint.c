/**
 * ============================================================================
 * File: test_wb_bigint.c
 * Module: Worklet 2 (bigint.c)
 * White-Box Testing Techniques Covered:
 *   - Statement Coverage: Every execution statement in bigint.c
 *   - Branch Coverage: Sign combinations, magnitude comparisons, borrow/carry
 *   - Path Coverage: Add/sub dispatch paths, comparison paths
 *   - Loop Testing: 0 iterations, 1, 2, typical (multi-limb), carry ripple
 * Invariants Tested:
 *   - Limb constraint: 0 <= digits[k] < 10^9
 *   - Canonical zero: sign = 0, size = 1, digits[0] = 0
 *   - Normalize removes all leading zero limbs
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "minitest.h"
#include "bigint.h"
#include "bigint_io.h"

int main(void) {
    minitest_init_suite("White-Box Unit: BigInteger (Worklet 2)", "tests/test_results.csv");

    TEST_CASE("UT-BIGINT-01", "UNIT", "Initialization, default canonical zero state, and free") {
        BigInteger a;
        bigint_init(&a);
        CHECK_INT_EQ(a.sign, 0, "Initial sign must be 0");
        CHECK_INT_EQ(a.size, 1, "Initial size must be 1 per canonical zero");
        CHECK(a.capacity >= 1, "Initial capacity must be at least 1");
        CHECK_NOT_NULL(a.digits, "Initial digits pointer must not be NULL");
        CHECK_INT_EQ(a.digits[0], 0, "Initial limb must be 0");
        bigint_free(&a);
        CHECK_INT_EQ(a.sign, 0, "Freed sign must be 0");
        CHECK_INT_EQ(a.size, 0, "Freed size must be 0");
        CHECK_NULL(a.digits, "Freed digits pointer must be NULL");
    }

    TEST_CASE("UT-BIGINT-02", "UNIT", "Canonical zero invariant after normalization") {
        BigInteger a;
        bigint_init(&a);
        bigint_set_u64(&a, 0);
        bigint_normalize(&a);
        CHECK_INT_EQ(a.sign, 0, "Zero sign must be 0");
        CHECK_INT_EQ(a.size, 1, "Zero size must be 1 per Rulebook §4.1");
        CHECK_NOT_NULL(a.digits, "Zero digits must not be NULL");
        CHECK_INT_EQ(a.digits[0], 0, "Zero limb must be 0");
        CHECK(bigint_is_zero(&a), "bigint_is_zero must return true");
        bigint_free(&a);
    }

    TEST_CASE("UT-BIGINT-03", "UNIT", "Stripping multiple leading zero limbs") {
        BigInteger a;
        bigint_init(&a);
        bigint_reserve(&a, 4);
        a.size = 4;
        a.sign = 1;
        a.digits[0] = 42;
        a.digits[1] = 0;
        a.digits[2] = 0;
        a.digits[3] = 0;
        bigint_normalize(&a);
        CHECK_INT_EQ(a.size, 1, "Size after stripping 3 leading zeros must be 1");
        CHECK_INT_EQ(a.digits[0], 42, "Limb 0 must retain 42");
        CHECK_INT_EQ(a.sign, 1, "Sign must remain 1");
        bigint_free(&a);
    }

    TEST_CASE("UT-BIGINT-04", "UNIT", "Deep-copy semantics: value_copy and independence") {
        BigInteger a, b;
        bigint_init(&a);
        bigint_init(&b);
        bigint_set_u64(&a, 9876543210ULL);
        CHECK(bigint_copy(&b, &a), "bigint_copy must succeed");
        CHECK_INT_EQ(b.sign, a.sign, "Signs must match");
        CHECK_INT_EQ(b.size, a.size, "Sizes must match");
        CHECK(a.digits != b.digits, "Digits buffers must be distinct heap allocations");
        CHECK_INT_EQ(b.digits[0], a.digits[0], "Limb 0 must match");
        /* Mutate a, ensure b is untouched */
        a.digits[0] = 111111;
        CHECK(b.digits[0] != a.digits[0], "Mutating original must not affect deep copy");
        bigint_free(&a);
        bigint_free(&b);
    }

    TEST_CASE("UT-BIGINT-05", "UNIT", "Magnitude comparison: less, equal, greater") {
        BigInteger a, b;
        bigint_init(&a);
        bigint_init(&b);
        bigint_set_u64(&a, 100);
        bigint_set_u64(&b, 200);
        CHECK_INT_EQ(bigint_compare_abs(&a, &b), -1, "100 < 200 magnitude");
        CHECK_INT_EQ(bigint_compare_abs(&b, &a), 1, "200 > 100 magnitude");
        CHECK_INT_EQ(bigint_compare_abs(&a, &a), 0, "100 == 100 magnitude");

        /* Multi-limb comparison */
        bigint_from_string(&a, "1000000000000000000"); /* 2 limbs */
        bigint_from_string(&b, "999999999999999999");  /* 2 limbs */
        CHECK_INT_EQ(bigint_compare_abs(&a, &b), 1, "Multi-limb 10^18 > 10^18-1");
        bigint_free(&a);
        bigint_free(&b);
    }

    TEST_CASE("UT-BIGINT-06", "UNIT", "Signed comparison: negative vs positive vs zero") {
        BigInteger a, b;
        bigint_init(&a);
        bigint_init(&b);
        bigint_from_string(&a, "-500");
        bigint_from_string(&b, "500");
        CHECK_INT_EQ(bigint_compare(&a, &b), -1, "-500 < 500");
        CHECK_INT_EQ(bigint_compare(&b, &a), 1, "500 > -500");
        bigint_set_u64(&b, 0);
        CHECK_INT_EQ(bigint_compare(&a, &b), -1, "-500 < 0");
        CHECK_INT_EQ(bigint_compare(&b, &a), 1, "0 > -500");
        bigint_free(&a);
        bigint_free(&b);
    }

    TEST_CASE("UT-BIGINT-07", "UNIT", "Addition: Carry ripple across multiple limbs") {
        BigInteger a, b, res;
        bigint_init(&a);
        bigint_init(&b);
        bigint_init(&res);
        /* 999999999 + 1 -> 1000000000 */
        bigint_from_string(&a, "999999999");
        bigint_from_string(&b, "1");
        CHECK(bigint_add(&res, &a, &b), "Addition must succeed");
        char *s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "1000000000", "Carry ripple across limb boundary");
        free(s);

        /* 3 consecutive 9s limbs + 1 */
        bigint_from_string(&a, "999999999999999999999999999");
        bigint_add(&res, &a, &b);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "1000000000000000000000000000", "Carry ripple across 3 limbs");
        free(s);

        bigint_free(&a);
        bigint_free(&b);
        bigint_free(&res);
    }

    TEST_CASE("UT-BIGINT-08", "UNIT", "Subtraction: Borrow ripple across multiple limbs") {
        BigInteger a, b, res;
        bigint_init(&a);
        bigint_init(&b);
        bigint_init(&res);
        bigint_from_string(&a, "1000000000000000000");
        bigint_from_string(&b, "1");
        CHECK(bigint_sub(&res, &a, &b), "Subtraction must succeed");
        char *s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "999999999999999999", "Borrow ripple 10^18 - 1");
        free(s);
        bigint_free(&a);
        bigint_free(&b);
        bigint_free(&res);
    }

    TEST_CASE("UT-BIGINT-09", "UNIT", "Branch coverage: All 4 signed add/sub combinations") {
        BigInteger a, b, res;
        bigint_init(&a); bigint_init(&b); bigint_init(&res);

        /* (+a) + (-b) = a - b */
        bigint_from_string(&a, "100");
        bigint_from_string(&b, "-40");
        bigint_add(&res, &a, &b);
        char *s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "60", "(+100) + (-40) = 60");
        free(s);

        /* (-a) + (+b) = b - a */
        bigint_from_string(&a, "-100");
        bigint_from_string(&b, "40");
        bigint_add(&res, &a, &b);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "-60", "(-100) + (+40) = -60");
        free(s);

        /* (-a) + (-b) = -(a + b) */
        bigint_from_string(&a, "-100");
        bigint_from_string(&b, "-40");
        bigint_add(&res, &a, &b);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "-140", "(-100) + (-40) = -140");
        free(s);

        /* (+a) - (-b) = a + b */
        bigint_from_string(&a, "100");
        bigint_from_string(&b, "-40");
        bigint_sub(&res, &a, &b);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "140", "(+100) - (-40) = 140");
        free(s);

        bigint_free(&a); bigint_free(&b); bigint_free(&res);
    }

    TEST_CASE("UT-BIGINT-10", "UNIT", "Schoolbook multiplication O(n^2)") {
        BigInteger a, b, res;
        bigint_init(&a); bigint_init(&b); bigint_init(&res);
        bigint_from_string(&a, "123456789");
        bigint_from_string(&b, "987654321");
        bigint_mul_schoolbook(&res, &a, &b);
        char *s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "121932631112635269", "123456789 * 987654321 exact");
        free(s);

        /* Multiplication with zero */
        bigint_set_u64(&b, 0);
        bigint_mul_schoolbook(&res, &a, &b);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "0", "N * 0 = 0");
        free(s);

        bigint_free(&a); bigint_free(&b); bigint_free(&res);
    }

    TEST_CASE("UT-BIGINT-11", "UNIT", "Limb invariant verification: all digits < 10^9") {
        BigInteger a, b, res;
        bigint_init(&a); bigint_init(&b); bigint_init(&res);
        bigint_from_string(&a, "999999999999999999999999999999");
        bigint_from_string(&b, "888888888888888888888888888888");
        bigint_add(&res, &a, &b);
        for (size_t i = 0; i < res.size; i++) {
            CHECK(res.digits[i] < 1000000000U, "Every limb must be strictly less than 10^9");
        }
        bigint_free(&a); bigint_free(&b); bigint_free(&res);
    }

    return minitest_finish_suite();
}
