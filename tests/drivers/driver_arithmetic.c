/**
 * ============================================================================
 * File: driver_arithmetic.c
 * Purpose: Test driver harness for bottom-up integration testing.
 * Role:
 *   Drives lower-level BigInteger and BigDecimal modules by feeding structured
 *   numeric vectors, simulating operations that upper layers (evaluator) would
 *   request, and validating exact results across module interfaces.
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bigint.h"
#include "bigint_io.h"
#include "decimal.h"
#include "fast_math.h"

int driver_run_bigint_pipeline(const char *s_a, const char *op, const char *s_b, char **out_str) {
    BigInteger a, b, res;
    bigint_init(&a);
    bigint_init(&b);
    bigint_init(&res);

    if (!bigint_from_string(&a, s_a) || !bigint_from_string(&b, s_b)) {
        bigint_free(&a);
        bigint_free(&b);
        bigint_free(&res);
        return 0;
    }

    int ok = 1;
    if (strcmp(op, "+") == 0) {
        ok = bigint_add(&res, &a, &b);
    } else if (strcmp(op, "-") == 0) {
        ok = bigint_sub(&res, &a, &b);
    } else if (strcmp(op, "*") == 0) {
        ok = bigint_mul(&res, &a, &b);
    } else if (strcmp(op, "/") == 0) {
        DivisionResult dr;
        division_result_init(&dr);
        ok = bigint_divide(&dr, &a, &b);
        if (ok) {
            bigint_copy(&res, &dr.quotient);
        }
        division_result_free(&dr);
    } else {
        ok = 0;
    }

    if (ok) {
        *out_str = bigint_to_string(&res);
    }

    bigint_free(&a);
    bigint_free(&b);
    bigint_free(&res);
    return ok;
}

int driver_run_decimal_pipeline(const char *s_a, const char *s_b, int precision, char **out_str) {
    BigDecimal da, db, res;
    decimal_init(&da);
    decimal_init(&db);
    decimal_init(&res);

    if (!decimal_from_string(&da, s_a) || !decimal_from_string(&db, s_b)) {
        decimal_free(&da);
        decimal_free(&db);
        decimal_free(&res);
        return 0;
    }

    int ok = decimal_div(&res, &da, &db, precision);
    if (ok) {
        *out_str = decimal_to_string(&res);
    }

    decimal_free(&da);
    decimal_free(&db);
    decimal_free(&res);
    return ok;
}
