/**
 * ============================================================================
 * File: test_wb_eval.c
 * Module: Worklet 3 (eval.c)
 * White-Box Testing Techniques Covered:
 *   - Statement & branch coverage for eval_node, eval_binary, eval_unary, eval_call
 *   - Dynamic type promotion ladder (VAL_INT -> VAL_DEC -> VAL_COMPLEX)
 *   - Exact vs remainder division type promotion (Rulebook §12)
 *   - MAX_RECURSION_DEPTH guard (128 levels) against stack overflow
 *   - Angular mode switching (ANGLE_RADIAN vs ANGLE_DEGREE)
 *   - Error latching and diagnostics (ERR_DIV_BY_ZERO, ERR_RECURSION_LIMIT)
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "eval.h"
#include "parser.h"

int main(void) {
    minitest_init_suite("White-Box Unit: AST Evaluator & Type Promotion (Worklet 3)", "tests/test_results.csv");

    TEST_CASE("UT-EV-01", "UNIT", "Integer arithmetic: 15 + 27 = 42 (VAL_INT)") {
        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("15 + 27", &pe), "Parse 15 + 27");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval_node succeeds");
        CHECK_INT_EQ(result.type, VAL_INT, "Result is VAL_INT");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "42", "15 + 27 = 42");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-02", "UNIT", "Type promotion: INT + DEC -> DEC (10 + 2.5 = 12.5)") {
        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("10 + 2.5", &pe), "Parse 10 + 2.5");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval_node succeeds");
        CHECK_INT_EQ(result.type, VAL_DEC, "Result promoted to VAL_DEC");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "12.5", "10 + 2.5 = 12.5");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-03", "UNIT", "Division promotion: 1 / 4 produces 0.25 (VAL_DEC)") {
        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.precision = 10;

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("1 / 4", &pe), "Parse 1 / 4");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval_node succeeds");
        CHECK_INT_EQ(result.type, VAL_DEC, "Integer division with remainder produces VAL_DEC");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "0.2500000000", "1 / 4 with precision 10 produces 0.2500000000");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-04", "UNIT", "Division by zero latches ERR_DIV_BY_ZERO") {
        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("10 / 0", &pe), "Parse 10 / 0");

        Value result;
        value_init(&result, VAL_INT);
        int ok = eval_node(pe.root, &ctx, &result);
        CHECK(!ok, "eval_node must fail on division by zero");
        CHECK_INT_EQ(ctx.error.code, ERR_DIV_BY_ZERO, "Error code must be ERR_DIV_BY_ZERO");

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-05", "UNIT", "Angular mode: sin(90) in ANGLE_DEGREE produces 1") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.angle_mode = ANGLE_DEGREE;
        ctx.precision = 10;

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sin(90)", &pe), "Parse sin(90)");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval sin(90) in degree mode");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "1.0000000000", "sin(90 deg) with precision 10 == 1.0000000000");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-06", "UNIT", "Recursion depth limit guard prevents stack overflow") {
        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        /* Manually simulate deep recursion depth */
        ctx.depth = MAX_RECURSION_DEPTH + 1;

        ExprNode *dummy = node_new_number("42", 1);
        Value result;
        value_init(&result, VAL_INT);

        int ok = eval_node(dummy, &ctx, &result);
        CHECK(!ok, "eval_node must reject when depth exceeds MAX_RECURSION_DEPTH");
        CHECK_INT_EQ(ctx.error.code, ERR_RECURSION_LIMIT, "Latches ERR_RECURSION_LIMIT");

        free_expr_node(dummy);
        value_free(&result);
        symtab_free(&st);
    }

    TEST_CASE("UT-EV-07", "UNIT", "Complex multiplication: i * i evaluates to -1") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("i * i", &pe), "Parse i * i");

        Value result;
        value_init(&result, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &result), "eval i * i");

        char *s = value_to_string(&result);
        CHECK_STR_EQ(s, "-1", "i * i = -1");
        free(s);

        value_free(&result);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    return minitest_finish_suite();
}
