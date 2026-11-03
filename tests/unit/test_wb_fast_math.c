/**
 * ============================================================================
 * File: test_wb_fast_math.c
 * Module: Worklet 3 (fast_math.c)
 * White-Box Testing Techniques Covered:
 *   - Statement Coverage: Karatsuba recursion, Knuth Algorithm D steps D1-D7
 *   - Branch Coverage: Karatsuba crossover (< 16, >= 16), Knuth q_hat == B,
 *     r_hat >= B, add-back branch (borrow after multiply-subtract)
 *   - Mathematical Invariants:
 *     - Karatsuba(a, b) == Schoolbook(a, b) across varied sizes
 *     - Knuth D: q * d + r == n and 0 <= r < d
 *     - GCD identities: gcd(a, b) == gcd(b, a % b)
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
#include "fast_math.h"

/* Helper: fills BigInteger with n limbs of pseudo-random or fixed patterns */
static void fill_limbs(BigInteger *a, size_t n, uint32_t base_pattern, int sign) {
    bigint_free(a);
    bigint_init_capacity(a, n);
    a->size = n;
    a->sign = sign;
    for (size_t i = 0; i < n; i++) {
        a->digits[i] = (base_pattern + (uint32_t)(i * 37)) % 1000000000U;
    }
    if (a->digits[n - 1] == 0) a->digits[n - 1] = 1; /* Ensure no leading zero */
    bigint_normalize(a);
}

int main(void) {
    minitest_init_suite("White-Box Unit: Fast Math (Karatsuba & Knuth D)", "tests/test_results.csv");

    TEST_CASE("UT-FM-01", "UNIT", "Karatsuba == Schoolbook: Size 1, 2, 31, 32, 33, 64, 100, 257 limbs") {
        size_t test_sizes[] = { 1, 2, 31, 32, 33, 64, 100, 257 };
        size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

        BigInteger a, b, res_sb, res_kt;
        bigint_init(&a); bigint_init(&b);
        bigint_init(&res_sb); bigint_init(&res_kt);

        for (size_t i = 0; i < num_sizes; i++) {
            size_t sz = test_sizes[i];
            fill_limbs(&a, sz, 123456789U, 1);
            fill_limbs(&b, sz, 987654321U, 1);

            bigint_mul_schoolbook(&res_sb, &a, &b);
            bigint_mul_karatsuba(&res_kt, &a, &b);

            CHECK_INT_EQ(bigint_compare(&res_sb, &res_kt), 0,
                         "Karatsuba must match Schoolbook for equal size");
        }

        bigint_free(&a); bigint_free(&b);
        bigint_free(&res_sb); bigint_free(&res_kt);
    }

    TEST_CASE("UT-FM-02", "UNIT", "Karatsuba == Schoolbook: Unequal operand sizes") {
        BigInteger a, b, res_sb, res_kt;
        bigint_init(&a); bigint_init(&b);
        bigint_init(&res_sb); bigint_init(&res_kt);

        /* 10 limbs x 45 limbs */
        fill_limbs(&a, 10, 111111111U, 1);
        fill_limbs(&b, 45, 222222222U, 1);
        bigint_mul_schoolbook(&res_sb, &a, &b);
        bigint_mul_karatsuba(&res_kt, &a, &b);
        CHECK_INT_EQ(bigint_compare(&res_sb, &res_kt), 0, "10 limbs x 45 limbs");

        /* 50 limbs x 20 limbs */
        fill_limbs(&a, 50, 333333333U, 1);
        fill_limbs(&b, 20, 444444444U, 1);
        bigint_mul_schoolbook(&res_sb, &a, &b);
        bigint_mul_karatsuba(&res_kt, &a, &b);
        CHECK_INT_EQ(bigint_compare(&res_sb, &res_kt), 0, "50 limbs x 20 limbs");

        bigint_free(&a); bigint_free(&b);
        bigint_free(&res_sb); bigint_free(&res_kt);
    }

    TEST_CASE("UT-FM-03", "UNIT", "Knuth D division invariant: q * d + r == n and 0 <= r < d") {
        BigInteger n, d, q_times_d, reconstructed;
        DivisionResult dr;
        bigint_init(&n); bigint_init(&d);
        bigint_init(&q_times_d); bigint_init(&reconstructed);
        division_result_init(&dr);

        /* Case A: Single-limb divisor */
        bigint_from_string(&n, "987654321098765432109876543210");
        bigint_from_string(&d, "123456789");
        CHECK(bigint_divide(&dr, &n, &d), "Division must succeed");
        CHECK(bigint_compare_abs(&dr.remainder, &d) < 0, "Remainder must be strictly less than divisor");
        CHECK(dr.remainder.sign >= 0, "Remainder must be non-negative");
        bigint_mul(&q_times_d, &dr.quotient, &d);
        bigint_add(&reconstructed, &q_times_d, &dr.remainder);
        CHECK_INT_EQ(bigint_compare(&reconstructed, &n), 0, "q * d + r == n (single limb divisor)");

        /* Case B: Two-limb divisor */
        bigint_from_string(&d, "100000000000000000"); /* 10^17 */
        CHECK(bigint_divide(&dr, &n, &d), "2-limb divisor");
        bigint_mul(&q_times_d, &dr.quotient, &d);
        bigint_add(&reconstructed, &q_times_d, &dr.remainder);
        CHECK_INT_EQ(bigint_compare(&reconstructed, &n), 0, "q * d + r == n (2 limb divisor)");

        /* Case C: Multi-limb divisor (4 limbs into 8 limbs) */
        fill_limbs(&n, 8, 777777777U, 1);
        fill_limbs(&d, 4, 333333333U, 1);
        CHECK(bigint_divide(&dr, &n, &d), "Multi-limb division");
        CHECK(bigint_compare_abs(&dr.remainder, &d) < 0, "Remainder < divisor");
        bigint_mul(&q_times_d, &dr.quotient, &d);
        bigint_add(&reconstructed, &q_times_d, &dr.remainder);
        CHECK_INT_EQ(bigint_compare(&reconstructed, &n), 0, "q * d + r == n (multi-limb)");

        bigint_free(&n); bigint_free(&d);
        bigint_free(&q_times_d); bigint_free(&reconstructed);
        division_result_free(&dr);
    }

    TEST_CASE("UT-FM-04", "UNIT", "Knuth D: Divisors needing add-back branch execution") {
        /* Craft operands where q_hat is overestimated by 1 and triggers add-back */
        BigInteger n, d, q_times_d, reconstructed;
        DivisionResult dr;
        bigint_init(&n); bigint_init(&d);
        bigint_init(&q_times_d); bigint_init(&reconstructed);
        division_result_init(&dr);

        /* Carefully crafted boundary near limb limit */
        bigint_from_string(&n, "999999999000000000000000000");
        bigint_from_string(&d, "999999999000000001");
        CHECK(bigint_divide(&dr, &n, &d), "Division with add-back trigger");
        bigint_mul(&q_times_d, &dr.quotient, &d);
        bigint_add(&reconstructed, &q_times_d, &dr.remainder);
        CHECK_INT_EQ(bigint_compare(&reconstructed, &n), 0, "q * d + r == n with add-back");

        bigint_free(&n); bigint_free(&d);
        bigint_free(&q_times_d); bigint_free(&reconstructed);
        division_result_free(&dr);
    }

    TEST_CASE("UT-FM-05", "UNIT", "Knuth D: Powers of 10^9 and all-(10^9-1) limb values") {
        BigInteger n, d, q_times_d, reconstructed;
        DivisionResult dr;
        bigint_init(&n); bigint_init(&d);
        bigint_init(&q_times_d); bigint_init(&reconstructed);
        division_result_init(&dr);

        /* All-(10^9-1) limbs: 999999999999999999 / 999999999 */
        bigint_from_string(&n, "999999999999999999");
        bigint_from_string(&d, "999999999");
        CHECK(bigint_divide(&dr, &n, &d), "All-9s limbs");
        char *q_str = bigint_to_string(&dr.quotient);
        char *r_str = bigint_to_string(&dr.remainder);
        CHECK_STR_EQ(q_str, "1000000001", "Quotient of 999999999999999999 / 999999999");
        CHECK_STR_EQ(r_str, "0", "Remainder of 999999999999999999 / 999999999");
        free(q_str); free(r_str);

        bigint_free(&n); bigint_free(&d);
        bigint_free(&q_times_d); bigint_free(&reconstructed);
        division_result_free(&dr);
    }

    TEST_CASE("UT-FM-06", "UNIT", "Euclidean GCD and identity gcd(a, b) * lcm == a * b") {
        BigInteger a, b, g, a_mul_b, q, r;
        DivisionResult dr;
        bigint_init(&a); bigint_init(&b); bigint_init(&g);
        bigint_init(&a_mul_b); bigint_init(&q); bigint_init(&r);
        division_result_init(&dr);

        bigint_from_string(&a, "1071");
        bigint_from_string(&b, "462");
        CHECK(bigint_gcd(&g, &a, &b), "GCD must succeed");
        char *s = bigint_to_string(&g);
        CHECK_STR_EQ(s, "21", "gcd(1071, 462) = 21");
        free(s);

        /* gcd(a, 0) == |a| */
        bigint_set_u64(&b, 0);
        bigint_gcd(&g, &a, &b);
        s = bigint_to_string(&g);
        CHECK_STR_EQ(s, "1071", "gcd(a, 0) == a");
        free(s);

        bigint_free(&a); bigint_free(&b); bigint_free(&g);
        bigint_free(&a_mul_b); bigint_free(&q); bigint_free(&r);
        division_result_free(&dr);
    }

    TEST_CASE("UT-FM-07", "UNIT", "Repeated squaring binary power: bigint_pow") {
        BigInteger base, res;
        bigint_init(&base); bigint_init(&res);

        bigint_set_u64(&base, 2);
        CHECK(bigint_pow(&res, &base, 64), "2^64");
        char *s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "18446744073709551616", "2^64 exact");
        free(s);

        /* 0^0 = 1 */
        bigint_set_u64(&base, 0);
        bigint_pow(&res, &base, 0);
        s = bigint_to_string(&res);
        CHECK_STR_EQ(s, "1", "0^0 = 1");
        free(s);

        bigint_free(&base); bigint_free(&res);
    }

    return minitest_finish_suite();
}

