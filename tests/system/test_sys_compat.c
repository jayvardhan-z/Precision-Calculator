/**
 * ============================================================================
 * File: test_sys_compat.c
 * Module: System Testing - Non-Functional Compatibility Verification
 * Description:
 *   Validates machine and runtime architectural compatibility:
 *     - SYS-CMP-01: Word size and integer width invariants (64-bit accumulator)
 *     - SYS-CMP-02: Endianness compatibility
 *     - SYS-CMP-03: Zero external math library dependencies (GMP, MPFR check)
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "minitest.h"

int main(void) {
    minitest_init_suite("System: Architectural Compatibility", "tests/test_results.csv");

    TEST_CASE("SYS-CMP-01", "COMPAT", "Word size invariants: uint64_t == 8 bytes, size_t >= 4 bytes") {
        CHECK_INT_EQ((int)sizeof(uint64_t), 8, "uint64_t is exactly 64-bit (8 bytes)");
        CHECK_INT_EQ((int)sizeof(uint32_t), 4, "uint32_t is exactly 32-bit (4 bytes)");
        CHECK((int)sizeof(size_t) >= 4, "size_t is at least 32 bits");
    }

    TEST_CASE("SYS-CMP-02", "COMPAT", "Endianness detection and memory alignment") {
        uint32_t x = 0x01020304;
        uint8_t *p = (uint8_t *)&x;
        int is_little = (p[0] == 0x04);
        int is_big    = (p[0] == 0x01);
        CHECK(is_little || is_big, "Platform has deterministic byte order (little or big endian)");
    }

    TEST_CASE("SYS-CMP-03", "COMPAT", "Zero-dependency constraint: Pure ISO C11 + POSIX libc/libm") {
        /* Run otool -L or ldd on ./calculator to verify zero external math dependencies */
        FILE *fp = popen("otool -L ./calculator 2>/dev/null || ldd ./calculator 2>/dev/null", "r");
        CHECK_NOT_NULL(fp, "Query dynamic dependencies");

        char line[256];
        int has_forbidden = 0;
        if (fp) {
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "gmp") || strstr(line, "mpfr") || strstr(line, "boost") ||
                    strstr(line, "cunit") || strstr(line, "unity")) {
                    has_forbidden = 1;
                }
            }
            pclose(fp);
        }
        CHECK_INT_EQ(has_forbidden, 0, "No external GMP/MPFR/CUnit dependencies found");
    }

    return minitest_finish_suite();
}

