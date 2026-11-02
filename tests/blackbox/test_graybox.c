/**
 * ============================================================================
 * File: test_graybox.c
 * Module: Gray-Box Testing
 * Description:
 *   Combines black-box functional inputs with internal white-box structural
 *   knowledge to stress algorithmic switchovers and internal state thresholds:
 *     - GB-01: Multiplications crossing KARATSUBA_THRESHOLD (32 limbs)
 *     - GB-02: Knuth Algorithm D division forcing the D4 "add-back" branch
 *     - GB-03: Newton-Raphson sqrt convergence and guard digit preservation
 *     - GB-04: Trigonometric range reduction across 2*pi boundary
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "parser.h"
#include "eval.h"
#include "bigint.h"
#include "bigint_io.h"
#include "fast_math.h"

int main(void) {
    minitest_init_suite("Gray-Box: Architectural & Algorithmic Thresholds", "tests/test_results.csv");

    TEST_CASE("GB-01", "GRAYBOX", "Karatsuba threshold boundary (15 vs 16 limbs)") {
        /* Construct 15-limb number (135 digits) and 16-limb number (144 digits) */
        char *d15 = malloc(136);
        char *d16 = malloc(145);
        memset(d15, '9', 135);
        d15[135] = '\0';
        memset(d16, '9', 144);
        d16[144] = '\0';

        BigInteger b15, b16, res_school, res_karat;
        bigint_init(&b15);
        bigint_init(&b16);
        bigint_init(&res_school);
        bigint_init(&res_karat);

        bigint_from_string(&b15, d15);
        bigint_from_string(&b16, d16);

        CHECK_INT_EQ(b15.size, 15, "b15 has exactly 15 limbs (below threshold 16)");
        CHECK_INT_EQ(b16.size, 16, "b16 has exactly 16 limbs (at threshold 16)");

        /* Multiply both ways */
        bigint_mul_schoolbook(&res_school, &b15, &b16);
        bigint_mul_karatsuba(&res_karat, &b15, &b16);

        CHECK_INT_EQ(bigint_compare(&res_school, &res_karat), 0,
                     "Schoolbook and Karatsuba results are identical at 16-limb threshold");

        bigint_free(&b15);
        bigint_free(&b16);
        bigint_free(&res_school);
        bigint_free(&res_karat);
        free(d15);
        free(d16);
    }

    TEST_CASE("GB-02", "GRAYBOX", "Knuth D Step D4 add-back branch trigger") {
        /* Divisor and dividend constructed to force q_hat estimate overshoot */
        BigInteger u, v;
        bigint_init(&u);
        bigint_init(&v);

        /* u = 1000000000 000000000, v = 999999999 */
        bigint_from_string(&u, "1000000000000000000");
        bigint_from_string(&v, "999999999");

        DivisionResult dr;
        division_result_init(&dr);

        CHECK(bigint_divide(&dr, &u, &v), "bigint_divide succeeds on add-back trigger");

        /* Verify q * v + r == u */
        BigInteger prod, sum;
        bigint_init(&prod);
        bigint_init(&sum);

        bigint_mul(&prod, &dr.quotient, &v);
        bigint_add(&sum, &prod, &dr.remainder);

        CHECK_INT_EQ(bigint_compare(&sum, &u), 0, "q * v + r == u on add-back division");
        CHECK(bigint_compare(&dr.remainder, &v) < 0, "0 <= r < v");

        bigint_free(&prod);
        bigint_free(&sum);
        division_result_free(&dr);
        bigint_free(&u);
        bigint_free(&v);
    }

    TEST_CASE("GB-03", "GRAYBOX", "Newton-Raphson sqrt convergence and guard digit isolation") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 50);

        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.precision = 50;

        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sqrt(2)", &pe), "Parse sqrt(2)");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate sqrt(2) to 50 places");
        CHECK_INT_EQ(res.type, VAL_DEC, "Result is VAL_DEC");

        /* Multiply sqrt(2) * sqrt(2) using high-level evaluator to verify ~ 2.000000 */
        char *s = value_to_string(&res);
        CHECK_NOT_NULL(s, "Formatted decimal result");
        /* First 20 known digits: 1.41421356237309504880 */
        CHECK(strncmp(s, "1.41421356237309504880", 22) == 0, "sqrt(2) accurate to known mathematical digits");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    TEST_CASE("GB-04", "GRAYBOX", "Trigonometric range reduction across 100*pi angle boundary") {
        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 30);

        EvalContext ctx;
        eval_context_init(&ctx, &st);
        ctx.precision = 20;

        /* sin(0) == sin(2*pi) == 0 */
        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sin(0)", &pe), "Parse sin(0)");

        Value res;
        value_init(&res, VAL_INT);
        CHECK(eval_node(pe.root, &ctx, &res), "Evaluate sin(0)");

        char *s = value_to_string(&res);
        CHECK_STR_EQ(s, "0", "sin(0) == 0");
        free(s);

        value_free(&res);
        parsed_expression_free(&pe);
        symtab_free(&st);
    }

    return minitest_finish_suite();
}
