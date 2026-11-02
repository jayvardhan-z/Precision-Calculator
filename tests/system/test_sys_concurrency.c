/**
 * ============================================================================
 * File: test_sys_concurrency.c
 * Module: System Testing - POSIX Multithreaded Concurrency
 * Description:
 *   Spawns 8 concurrent POSIX threads, each running independent EvalContext
 *   instances evaluating distinct expressions:
 *     - Thread 0: 2 ^ 500
 *     - Thread 1: 500!
 *     - Thread 2: sqrt(123456789)
 *     - Thread 3: sin(0.5) + cos(0.5)
 *     - Thread 4: (10 + 20i) * (30 - 40i)
 *     - Thread 5: exp(2)
 *     - Thread 6: ln(100)
 *     - Thread 7: gcd(12345678, 87654321)
 *   Verifies absence of shared mutable state corruption across threads.
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "minitest.h"
#include "parser.h"
#include "eval.h"

#define NUM_THREADS 8

typedef struct {
    int thread_id;
    const char *expr;
    int success;
    char result_buf[256];
} ThreadArg;

static void *thread_worker(void *arg) {
    ThreadArg *targ = (ThreadArg *)arg;

    SymbolTable st;
    symtab_init(&st);
    symtab_preload_constants(&st, 20);

    EvalContext ctx;
    eval_context_init(&ctx, &st);
    ctx.precision = 20;

    ParsedExpression pe;
    parsed_expression_init(&pe);
    if (!parse_string_to_ast(targ->expr, &pe)) {
        parsed_expression_free(&pe);
        symtab_free(&st);
        targ->success = 0;
        return NULL;
    }

    Value val;
    value_init(&val, VAL_INT);

    if (eval_node(pe.root, &ctx, &val)) {
        targ->success = 1;
        char *s = value_to_string(&val);
        if (s) {
            strncpy(targ->result_buf, s, sizeof(targ->result_buf) - 1);
            targ->result_buf[sizeof(targ->result_buf) - 1] = '\0';
            free(s);
        }
    } else {
        targ->success = 0;
    }

    value_free(&val);
    parsed_expression_free(&pe);
    symtab_free(&st);

    return NULL;
}

int main(void) {
    minitest_init_suite("System: Multithreaded Concurrency (POSIX pthreads)", "tests/test_results.csv");

    TEST_CASE("SYS-CONC-01", "SYSTEM", "8 concurrent threads evaluate distinct numeric workloads safely") {
        pthread_t threads[NUM_THREADS];
        ThreadArg args[NUM_THREADS] = {
            { 0, "2 ^ 500", 0, "" },
            { 1, "500!", 0, "" },
            { 2, "sqrt(123456789)", 0, "" },
            { 3, "sin(0.5) + cos(0.5)", 0, "" },
            { 4, "(10 + 20*i) * (30 - 40*i)", 0, "" },
            { 5, "exp(2)", 0, "" },
            { 6, "ln(100)", 0, "" },
            { 7, "gcd(12345678, 87654321)", 0, "" }
        };

        for (int i = 0; i < NUM_THREADS; i++) {
            int rc = pthread_create(&threads[i], NULL, thread_worker, &args[i]);
            CHECK(rc == 0, "pthread_create succeeded");
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
            CHECK(args[i].success == 1, "Thread completed evaluation successfully");
            CHECK(strlen(args[i].result_buf) > 0, "Thread computed non-empty result");
        }
    }

    return minitest_finish_suite();
}

