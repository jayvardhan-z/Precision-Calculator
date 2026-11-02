/**
 * ============================================================================
 * File: minitest.h
 * Description: Zero-dependency ISO C11 / POSIX test framework for Precision-Calculator.
 * Features:
 *   - Per-test ID, name, classification type
 *   - Nanosecond-precision interval timing via clock_gettime(CLOCK_MONOTONIC)
 *   - Assertion macros: CHECK, CHECK_INT_EQ, CHECK_STR_EQ, CHECK_NOT_NULL, etc.
 *   - Machine-readable test execution telemetry: (ID,TYPE,RESULT,DURATION_MS,MESSAGE)
 *   - Suite summaries and non-zero exit code on failure
 *   - Integrated Linear Congruential Generator (LCG) for deterministic fuzzing
 * ============================================================================
 */

#ifndef MINITEST_H
#define MINITEST_H

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Test Execution Status */
typedef enum {
    TEST_RESULT_PASS = 0,
    TEST_RESULT_FAIL = 1,
    TEST_RESULT_SKIP = 2
} MiniTestResult;

/* Global Test Runner State */
typedef struct {
    const char *suite_name;
    size_t      total_count;
    size_t      pass_count;
    size_t      fail_count;
    size_t      skip_count;
    FILE       *log_file;       /* Optional machine-readable CSV stream */
    bool        verbose;
    struct timespec suite_start;
} MiniTestSuite;

/* Current Test Case Context */
typedef struct {
    const char *id;
    const char *type;           /* "UNIT", "INTEGRATION", "SYSTEM", "ACCEPTANCE", "BLACKBOX" */
    const char *description;
    struct timespec start_time;
    bool        failed;
    char        failure_msg[512];
} MiniTestCase;

static MiniTestSuite g_minitest_suite = {
    .suite_name = "Global Suite",
    .total_count = 0,
    .pass_count = 0,
    .fail_count = 0,
    .skip_count = 0,
    .log_file = NULL,
    .verbose = true
};

static MiniTestCase g_current_test = {0};

/* Monotonic time helper in milliseconds */
static inline double minitest_timespec_diff_ms(struct timespec start, struct timespec end) {
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec);
    return (sec * 1000.0) + (nsec / 1000000.0);
}

/* Suite Lifecycle */
static inline void minitest_init_suite(const char *name, const char *csv_log_path) {
    g_minitest_suite.suite_name = name;
    g_minitest_suite.total_count = 0;
    g_minitest_suite.pass_count = 0;
    g_minitest_suite.fail_count = 0;
    g_minitest_suite.skip_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &g_minitest_suite.suite_start);

    if (csv_log_path) {
        g_minitest_suite.log_file = fopen(csv_log_path, "a");
        if (g_minitest_suite.log_file) {
            /* If file is newly created, write header */
            fseek(g_minitest_suite.log_file, 0, SEEK_END);
            if (ftell(g_minitest_suite.log_file) == 0) {
                fprintf(g_minitest_suite.log_file, "ID,TYPE,RESULT,DURATION_MS,MESSAGE\n");
            }
        }
    }
    printf("=== Test Suite: %s ===\n", name);
}

static inline void minitest_start_test(const char *id, const char *type, const char *desc) {
    g_current_test.id = id;
    g_current_test.type = type;
    g_current_test.description = desc;
    g_current_test.failed = false;
    g_current_test.failure_msg[0] = '\0';
    clock_gettime(CLOCK_MONOTONIC, &g_current_test.start_time);
}

static inline void minitest_end_test(void) {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_ms = minitest_timespec_diff_ms(g_current_test.start_time, end_time);

    g_minitest_suite.total_count++;
    if (g_current_test.failed) {
        g_minitest_suite.fail_count++;
        printf("  [\033[1;31mFAIL\033[0m] %s (%s): %s [%.3f ms]\n",
               g_current_test.id, g_current_test.type, g_current_test.description, elapsed_ms);
        printf("         Reason: %s\n", g_current_test.failure_msg);
        if (g_minitest_suite.log_file) {
            fprintf(g_minitest_suite.log_file, "%s,%s,FAIL,%.3f,\"%s\"\n",
                    g_current_test.id, g_current_test.type, elapsed_ms, g_current_test.failure_msg);
            fflush(g_minitest_suite.log_file);
        }
    } else {
        g_minitest_suite.pass_count++;
        if (g_minitest_suite.verbose) {
            printf("  [\033[1;32mPASS\033[0m] %s (%s): %s [%.3f ms]\n",
                   g_current_test.id, g_current_test.type, g_current_test.description, elapsed_ms);
        }
        if (g_minitest_suite.log_file) {
            fprintf(g_minitest_suite.log_file, "%s,%s,PASS,%.3f,\"OK\"\n",
                    g_current_test.id, g_current_test.type, elapsed_ms);
            fflush(g_minitest_suite.log_file);
        }
    }
}

static inline int minitest_finish_suite(void) {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_ms = minitest_timespec_diff_ms(g_minitest_suite.suite_start, end_time);

    printf("\n--- Suite Summary: %s ---\n", g_minitest_suite.suite_name);
    printf("  Total Tests: %zu\n", g_minitest_suite.total_count);
    printf("  Passed:      \033[1;32m%zu\033[0m\n", g_minitest_suite.pass_count);
    printf("  Failed:      \033[1;%sm%zu\033[0m\n",
           g_minitest_suite.fail_count > 0 ? "31" : "32", g_minitest_suite.fail_count);
    printf("  Duration:    %.3f ms\n", total_ms);

    if (g_minitest_suite.log_file) {
        fclose(g_minitest_suite.log_file);
        g_minitest_suite.log_file = NULL;
    }

    return (g_minitest_suite.fail_count == 0) ? 0 : 1;
}

/* ============================================================================
 * Assertions
 * ============================================================================ */

#define TEST_CASE(id, type, desc) \
    for (int _minitest_once = (minitest_start_test(id, type, desc), 1); \
         _minitest_once; \
         _minitest_once = 0, minitest_end_test())

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Assertion failed: %s (%s)", __FILE__, __LINE__, #cond, msg); \
    } \
} while(0)

#define CHECK_INT_EQ(actual, expected, msg) do { \
    long long _act = (long long)(actual); \
    long long _exp = (long long)(expected); \
    if (_act != _exp) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Expected %lld, got %lld (%s)", __FILE__, __LINE__, _exp, _act, msg); \
    } \
} while(0)

#define CHECK_UINT64_EQ(actual, expected, msg) do { \
    uint64_t _act = (uint64_t)(actual); \
    uint64_t _exp = (uint64_t)(expected); \
    if (_act != _exp) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Expected %llu, got %llu (%s)", __FILE__, __LINE__, \
                 (unsigned long long)_exp, (unsigned long long)_act, msg); \
    } \
} while(0)

#define CHECK_STR_EQ(actual, expected, msg) do { \
    const char *_act = (actual); \
    const char *_exp = (expected); \
    if (!_act || !_exp || strcmp(_act, _exp) != 0) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Expected \"%s\", got \"%s\" (%s)", __FILE__, __LINE__, \
                 _exp ? _exp : "NULL", _act ? _act : "NULL", msg); \
    } \
} while(0)

#define CHECK_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Expected non-null pointer (%s)", __FILE__, __LINE__, msg); \
    } \
} while(0)

#define CHECK_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { \
        g_current_test.failed = true; \
        snprintf(g_current_test.failure_msg, sizeof(g_current_test.failure_msg), \
                 "%s:%d: Expected NULL pointer (%s)", __FILE__, __LINE__, msg); \
    } \
} while(0)

/* ============================================================================
 * Deterministic Linear Congruential Generator (LCG)
 * Numerical Recipes parameters: m = 2^32, a = 1664525, c = 1013904223
 * ============================================================================ */
typedef struct {
    uint32_t state;
    uint32_t initial_seed;
} MiniLCG;

static inline void minitest_lcg_init(MiniLCG *lcg, uint32_t seed) {
    lcg->state = seed;
    lcg->initial_seed = seed;
}

static inline uint32_t minitest_lcg_next(MiniLCG *lcg) {
    lcg->state = lcg->state * 1664525u + 1013904223u;
    return lcg->state;
}

static inline uint32_t minitest_lcg_range(MiniLCG *lcg, uint32_t min, uint32_t max) {
    if (min >= max) return min;
    return min + (minitest_lcg_next(lcg) % (max - min + 1));
}

#ifdef __cplusplus
}
#endif

#endif /* MINITEST_H */
