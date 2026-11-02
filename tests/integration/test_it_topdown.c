/**
 * ============================================================================
 * File: test_it_topdown.c
 * Module: Integration Testing - Top-Down Approach
 * Description:
 *   Integrates high-level modules (Lexer, Parser, Evaluator) using Stubs
 *   for lower-level mathematical modules (stub_functions.c).
 *   Verifies:
 *     - High-level control flow reaches lower-level function interfaces
 *     - Argument passing and return value unpacking across module boundaries
 *     - Stub error propagation (domain error handling in evaluator)
 *     - Call counting instrumentation
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "eval.h"
#include "parser.h"

// External declarations from stub_functions.c
extern int g_stub_sin_calls;
extern int g_stub_cos_calls;
extern int g_stub_sqrt_calls;
extern int g_stub_force_error;
extern void stub_functions_reset(void);

int main(void) {
    minitest_init_suite("Integration: Incremental Top-Down (Stubs)", "tests/test_results.csv");

    TEST_CASE("IT-TD-01", "INTEG", "Top-down call dispatch to stubbed sqrt(4)") {
        stub_functions_reset();
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sqrt(4)", &pe), "Parse sqrt(4)");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval_node calls stub_sqrt");
        CHECK_INT_EQ(g_stub_sqrt_calls, 1, "Stub sqrt was invoked exactly once");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "2", "Stub returned 2 for sqrt(4)");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("IT-TD-02", "INTEG", "Top-down call dispatch to stubbed sin(1.57)") {
        stub_functions_reset();
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sin(1.57)", &pe), "Parse sin(1.57)");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval_node calls stub_sin");
        CHECK_INT_EQ(g_stub_sin_calls, 1, "Stub sin was invoked exactly once");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "0.5", "Stub returned 0.5 for sin");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("IT-TD-03", "INTEG", "Top-down error propagation when lower stub fails") {
        stub_functions_reset();
        g_stub_force_error = 1; // Force stub to return 0 (simulated domain/internal failure)

        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sqrt(4)", &pe), "Parse sqrt(4)");

        Value result;
        value_init(&result, VAL_INT);
        int ok = eval_node(pe.root, &ctx, &result);
        CHECK(!ok, "eval_node must fail when lower-level stub returns failure");
        CHECK(ctx.error.code != ERR_NONE, "Evaluator latched error code");

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    return minitest_finish_suite();
}

