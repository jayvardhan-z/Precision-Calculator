/**
 * ============================================================================
 * File: test_sys_perf.c
 * Module: System Testing - Non-Functional Performance Benchmarking
 * Description:
 *   Validates execution latency against tests/data/perf_baseline.csv SLAs:
 *     - PERF-01: 2 ^ 100000 binary repeated squaring (< 1500 ms)
 *     - PERF-02: 10000! factorial (35660 digits) (< 3000 ms)
 *     - PERF-03: 5000-digit x 5000-digit Karatsuba multiplication (< 500 ms)
 *     - PERF-04: 5000-digit / 2500-digit Knuth D division (< 1000 ms)
 *     - PERF-05: sqrt(2) to 2000 decimal places (< 800 ms)
 *     - PERF-06: pi constant to 200 decimal places (< 500 ms)
 *     - PERF-07: exp(1) constant to 200 decimal places (< 500 ms)
 *     - PERF-08: ln(2) constant to 200 decimal places (< 600 ms)
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "minitest.h"
#include "timer.h"
#include "parser.h"
#include "eval.h"

static double measure_eval(const char *expr, int precision) {
    SymbolTable st;
    symtab_init(&st);
    symtab_preload_constants(&st, precision);

    EvalContext ctx;
    eval_context_init(&ctx, &st);
    ctx.precision = precision;

    ParsedExpression pe;
    parsed_expression_init(&pe);
    if (!parse_string_to_ast(expr, &pe)) {
        parsed_expression_free(&pe);
        symtab_free(&st);
        return -1.0;
    }

    Value res;
    value_init(&res, VAL_INT);

    Timer t;
    timer_start(&t);
    int ok = eval_node(pe.root, &ctx, &res);
    timer_stop(&t);

    double ms = timer_elapsed_ms(&t);

    value_free(&res);
    parsed_expression_free(&pe);
    symtab_free(&st);

    return ok ? ms : -1.0;
}

int main(void) {
    minitest_init_suite("System: Performance Benchmarking vs SLAs", "tests/test_results.csv");

    TEST_CASE("PERF-01", "PERF", "2 ^ 100000 repeated squaring latency (< 1500 ms)") {
        double ms = measure_eval("2 ^ 100000", 20);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 1500.0, "Latency must be <= 1500 ms");
    }

    TEST_CASE("PERF-02", "PERF", "10000! factorial calculation latency (< 3000 ms)") {
        double ms = measure_eval("10000!", 20);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 3000.0, "Latency must be <= 3000 ms");
    }

    TEST_CASE("PERF-03", "PERF", "5000-digit x 5000-digit Karatsuba multiplication (< 500 ms)") {
        /* Generate two 5000-digit numbers: 9 repeated 5000 times */
        char *d1 = malloc(5001);
        char *d2 = malloc(5001);
        memset(d1, '9', 5000);
        d1[5000] = '\0';
        memset(d2, '7', 5000);
        d2[5000] = '\0';

        char *expr = malloc(10050);
        snprintf(expr, 10050, "%s * %s", d1, d2);

        double ms = measure_eval(expr, 20);
        free(d1);
        free(d2);
        free(expr);

        CHECK(ms >= 0.0, "Multiplication succeeded");
        CHECK(ms <= 500.0, "Latency must be <= 500 ms");
    }

    TEST_CASE("PERF-04", "PERF", "5000-digit / 2500-digit Knuth D division (< 1000 ms)") {
        char *d1 = malloc(5001);
        char *d2 = malloc(2501);
        memset(d1, '8', 5000);
        d1[5000] = '\0';
        memset(d2, '3', 2500);
        d2[2500] = '\0';

        char *expr = malloc(8000);
        snprintf(expr, 8000, "%s / %s", d1, d2);

        double ms = measure_eval(expr, 20);
        free(d1);
        free(d2);
        free(expr);

        CHECK(ms >= 0.0, "Division succeeded");
        CHECK(ms <= 1000.0, "Latency must be <= 1000 ms");
    }

    TEST_CASE("PERF-05", "PERF", "sqrt(2) to 2000 decimal places (< 800 ms)") {
        double ms = measure_eval("sqrt(2)", 2000);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 800.0, "Latency must be <= 800 ms");
    }

    TEST_CASE("PERF-06", "PERF", "pi calculation to 200 decimal places (< 500 ms)") {
        double ms = measure_eval("pi", 200);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 500.0, "Latency must be <= 500 ms");
    }

    TEST_CASE("PERF-07", "PERF", "exp(1) calculation to 200 decimal places (< 500 ms)") {
        double ms = measure_eval("exp(1)", 200);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 500.0, "Latency must be <= 500 ms");
    }

    TEST_CASE("PERF-08", "PERF", "ln(2) calculation to 200 decimal places (< 600 ms)") {
        double ms = measure_eval("ln(2)", 200);
        CHECK(ms >= 0.0, "Evaluation succeeded");
        CHECK(ms <= 600.0, "Latency must be <= 600 ms");
    }

    return minitest_finish_suite();
}

