/**
 * ============================================================================
 * File: test_wb_bigint_io.c
 * Module: Worklet 1 (bigint_io.c)
 * White-Box Testing Techniques Covered:
 *   - 9-digit chunk boundary serialization (8, 9, 10, 18, 27 digits)
 *   - String <-> BigInteger roundtrip invariance
 *   - Stripping leading zero characters during string parsing
 *   - Zero string formatting ("0", "+0", "-0")
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "bigint.h"
#include "bigint_io.h"

int main(void) {
    minitest_init_suite("White-Box Unit: BigInt I/O Serialization", "tests/test_results.csv");

    TEST_CASE("UT-IO-01", "UNIT", "Zero string variations normalize to canonical zero '0'") {
        const char *zero_strings[] = { "0", "+0", "-0", "0000", "+000000", "-000" };
        size_t n = sizeof(zero_strings) / sizeof(zero_strings[0]);
        for (size_t i = 0; i < n; i++) {
            BigInteger a;
            bigint_init(&a);
            CHECK(bigint_from_string(&a, zero_strings[i]), "Parse zero string");
            char *out = bigint_to_string(&a);
            CHECK_STR_EQ(out, "0", "All zero variants must serialize to '0'");
            CHECK_INT_EQ(a.sign, 0, "Canonical zero sign must be 0");
            CHECK_INT_EQ(a.size, 1, "Canonical zero size must be 1");
            free(out);
            bigint_free(&a);
        }
    }

    TEST_CASE("UT-IO-02", "UNIT", "9-digit chunk boundaries: 8, 9, 10, 18, 27 digits") {
        const char *test_numbers[] = {
            "12345678",                         /* 8 digits: fits inside 1 limb with leading 0 */
            "999999999",                        /* 9 digits: exactly fills 1 limb */
            "1000000000",                       /* 10 digits: 2 limbs (1 and 000000000) */
            "123456789987654321",               /* 18 digits: exactly fills 2 limbs */
            "1000000000000000000",              /* 19 digits: 3 limbs */
            "999999999999999999999999999"       /* 27 digits: exactly fills 3 limbs */
        };
        size_t n = sizeof(test_numbers) / sizeof(test_numbers[0]);

        for (size_t i = 0; i < n; i++) {
            BigInteger a;
            bigint_init(&a);
            CHECK(bigint_from_string(&a, test_numbers[i]), "Parse boundary string");
            char *out = bigint_to_string(&a);
            CHECK_STR_EQ(out, test_numbers[i], "Round-trip boundary check");
            free(out);
            bigint_free(&a);
        }
    }

    TEST_CASE("UT-IO-03", "UNIT", "Signed string roundtrip: positive and negative") {
        BigInteger a;
        bigint_init(&a);

        /* Negative number */
        bigint_from_string(&a, "-9876543210123456789");
        char *out = bigint_to_string(&a);
        CHECK_STR_EQ(out, "-9876543210123456789", "Negative serialization");
        free(out);

        /* Positive with explicit plus */
        bigint_from_string(&a, "+456789123");
        out = bigint_to_string(&a);
        CHECK_STR_EQ(out, "456789123", "Positive sign stripped in normalized serialization");
        free(out);

        bigint_free(&a);
    }

    TEST_CASE("UT-IO-04", "UNIT", "Stripping leading zeros on non-zero numbers") {
        BigInteger a;
        bigint_init(&a);
        bigint_from_string(&a, "00000000012345");
        char *out = bigint_to_string(&a);
        CHECK_STR_EQ(out, "12345", "Leading zeros must be stripped");
        free(out);
        bigint_free(&a);
    }

    return minitest_finish_suite();
}

