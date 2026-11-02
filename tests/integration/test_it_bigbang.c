/**
 * ============================================================================
 * File: test_it_bigbang.c
 * Module: Integration Testing - Non-Incremental Big-Bang Approach
 * Description:
 *   Combines all production modules simultaneously (Lexer, Parser, AST,
 *   Evaluator, BigInteger, FastMath, Decimal, Complex, Functions) without stubs.
 *   Verifies full multi-module integration across the entire compilation and
 *   execution pipeline.
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "parser.h"
#include "eval.h"

int main(void) {
    minitest_init_suite("Integration: Non-Incremental Big-Bang Smoke", "tests/test_results.csv");

    TEST_CASE("IT-BB-01", "INTEG", "Big-Bang: (2 + 3) * (4 + 5) ^ 2 evaluates to 405") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("(2 + 3) * (4 + 5) ^ 2", &pe), "Parse expression");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate expression");
        CHECK_INT_EQ(res.type, VAL_INT, "Result is VAL_INT");

        char *s = value_to_string(&res);
        CHECK_STR_EQ(s, "405", "Result matches 405");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("IT-BB-02", "INTEG", "Big-Bang: sqrt(25) + cos(0) evaluates to 6") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.precision = 5;

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sqrt(25) + cos(0)", &pe), "Parse sqrt(25) + cos(0)");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate transcendental expression");

        char *s = value_to_string(&res);
        /* In decimal precision 5, renders as 6.00000 */
        CHECK(strncmp(s, "6", 1) == 0, "Leading digit is 6");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("IT-BB-03", "INTEG", "Big-Bang: (2 + 3*i) * (2 - 3*i) evaluates to 13") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("(2 + 3*i) * (2 - 3*i)", &pe), "Parse complex expression");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate complex conjugate product");

        char *s = value_to_string(&res);
        CHECK_STR_EQ(s, "13", "(2 + 3*i)*(2 - 3*i) == 13");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("IT-BB-04", "INTEG", "Big-Bang: 1 / 8 produces 0.1250000000 at precision 10") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.precision = 10;

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("1 / 8", &pe), "Parse 1 / 8");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate division");

        char *s = value_to_string(&res);
        CHECK_STR_EQ(s, "0.1250000000", "Result matches 0.1250000000");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    return minitest_finish_suite();
}
