/**
 * ============================================================================
 * File: test_it_flow.c
 * Module: Integration Testing - End-to-End Flow Testing
 * Description:
 *   Simulates the complete interactive session workflow (analogous to the
 *   syllabus Login -> Compose -> Logout flow):
 *     1. Session Initialization (preloads constants, ans = 0)
 *     2. Operation 1: Evaluate 100 + 200 -> updates ans to 300
 *     3. Operation 2: Evaluate ans * 3 -> updates ans to 900
 *     4. Operation 3: Evaluate ans / 4 -> updates ans to 225
 *     5. Precision Mode Switch: precision changed to 5
 *     6. Operation 4: Evaluate ans / 2 -> produces 112.50000
 *     7. Error Recovery: Evaluate ans / 0 -> rejected, ans remains 112.50000
 *     8. Session Termination: clean deallocation with zero leaks
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "parser.h"
#include "eval.h"

// Helper to evaluate an expression string within a stateful session and update 'ans'
static int session_evaluate(const char *expr, EvalContext *ctx, SymbolTable *st, char **out_str) {
    ParsedExpression pe;
    parsed_expression_init(&pe);

    if (!parse_string_to_ast(expr, &pe)) {
        parsed_expression_free(&pe);
        return 0;
    }

    Value result;
    value_init(&result, VAL_INT);

    if (!eval_node(pe.root, ctx, &result)) {
        value_free(&result);
        parsed_expression_free(&pe);
        return 0;
    }

    /* Update 'ans' on successful evaluation */
    symtab_set(st, "ans", &result);

    if (out_str) {
        *out_str = value_to_string(&result);
    }

    value_free(&result);
    parsed_expression_free(&pe);
    return 1;
}

int main(void) {
    minitest_init_suite("Integration: End-to-End Stateful Flow", "tests/test_results.csv");

    SymbolTable st;
    symtab_init(&st);
    symtab_preload_constants(&st, 20);

    EvalContext ctx;
    eval_context_init(&ctx, &st);
    ctx.precision = 20;

    TEST_CASE("IT-FLOW-01", "INTEG", "Flow Step 1: Initial ans is 0") {
        Value v_ans;
        value_init(&v_ans, VAL_INT);
        CHECK(symtab_get(&st, "ans", &v_ans), "ans exists in symtab");
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "0", "Initial ans is 0");
        free(s);
        value_free(&v_ans);
    }

    TEST_CASE("IT-FLOW-02", "INTEG", "Flow Step 2: Compute 100 + 200 -> ans becomes 300") {
        char *out = NULL;
        CHECK(session_evaluate("100 + 200", &ctx, &st, &out), "Evaluate 100 + 200");
        CHECK_STR_EQ(out, "300", "Result is 300");
        free(out);

        Value v_ans;
        value_init(&v_ans, VAL_INT);
        symtab_get(&st, "ans", &v_ans);
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "300", "ans updated to 300");
        free(s);
        value_free(&v_ans);
    }

    TEST_CASE("IT-FLOW-03", "INTEG", "Flow Step 3: Compute ans * 3 -> ans becomes 900") {
        char *out = NULL;
        CHECK(session_evaluate("ans * 3", &ctx, &st, &out), "Evaluate ans * 3");
        CHECK_STR_EQ(out, "900", "Result is 900");
        free(out);

        Value v_ans;
        value_init(&v_ans, VAL_INT);
        symtab_get(&st, "ans", &v_ans);
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "900", "ans updated to 900");
        free(s);
        value_free(&v_ans);
    }

    TEST_CASE("IT-FLOW-04", "INTEG", "Flow Step 4: Compute ans / 4 -> ans becomes 225") {
        char *out = NULL;
        CHECK(session_evaluate("ans / 4", &ctx, &st, &out), "Evaluate ans / 4");
        CHECK_STR_EQ(out, "225", "Result is 225");
        free(out);

        Value v_ans;
        value_init(&v_ans, VAL_INT);
        symtab_get(&st, "ans", &v_ans);
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "225", "ans updated to 225");
        free(s);
        value_free(&v_ans);
    }

    TEST_CASE("IT-FLOW-05", "INTEG", "Flow Step 5 & 6: Set precision 5, compute ans / 2 -> produces 112.50000") {
        ctx.precision = 5;
        char *out = NULL;
        CHECK(session_evaluate("ans / 2", &ctx, &st, &out), "Evaluate ans / 2");
        CHECK_STR_EQ(out, "112.50000", "Result is 112.50000");
        free(out);

        Value v_ans;
        value_init(&v_ans, VAL_INT);
        symtab_get(&st, "ans", &v_ans);
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "112.50000", "ans updated to 112.50000");
        free(s);
        value_free(&v_ans);
    }

    TEST_CASE("IT-FLOW-06", "INTEG", "Flow Step 7: Error recovery preserves ans unchanged") {
        char *out = NULL;
        int ok = session_evaluate("ans / 0", &ctx, &st, &out);
        CHECK(!ok, "Evaluation fails on division by zero");

        /* Verify ans was NOT altered by failed evaluation */
        Value v_ans;
        value_init(&v_ans, VAL_INT);
        symtab_get(&st, "ans", &v_ans);
        char *s = value_to_string(&v_ans);
        CHECK_STR_EQ(s, "112.50000", "ans remains 112.50000 after division by zero failure");
        free(s);
        value_free(&v_ans);
    }

    symtab_free(&st);

    return minitest_finish_suite();
}

