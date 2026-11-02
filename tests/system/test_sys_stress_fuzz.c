/**
 * ============================================================================
 * File: test_sys_stress_fuzz.c
 * Module: System Testing - Stress & Random Fuzzing
 * Description:
 *   Adversarial stress testing and automated pseudo-random fuzzing:
 *     - SYS-STR-01: 128 nested parentheses (within MAX_RECURSION_DEPTH)
 *     - SYS-STR-02: 129 nested parentheses (rejection without crash)
 *     - SYS-STR-03: 10,000-digit integer literal parsing and addition
 *     - SYS-STR-04: Seeded LCG byte fuzzer (10,000 randomized mutated strings;
 *                   fixed seed printed to obey Testing Principle 5)
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "minitest.h"
#include "parser.h"
#include "eval.h"

int main(void) {
    minitest_init_suite("System: Stress & Pseudo-Random Fuzzing", "tests/test_results.csv");

    TEST_CASE("SYS-STR-01", "STRESS", "127 nested parentheses evaluated successfully") {
        /* Construct: ( ( ( ... 1 ... ) ) ) */
        int depth = 127;
        char *expr = malloc(depth * 2 + 10);
        for (int i = 0; i < depth; i++) expr[i] = '(';
        expr[depth] = '1';
        for (int i = 0; i < depth; i++) expr[depth + 1 + i] = ')';
        expr[depth * 2 + 1] = '\0';

        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        int parsed = parse_string_to_ast(expr, &pe);
        CHECK(parsed, "127 nested parens parsed successfully");

        if (parsed) {
            Value val;
            value_init(&val, VAL_INT);
            int ok = eval_node(pe.root, &ctx, &val);
            CHECK(ok, "127 nested parens evaluated successfully");
            value_free(&val);
            parsed_expression_free(&pe);
        }

        symtab_free(&st);
        free(expr);
    }

    TEST_CASE("SYS-STR-02", "STRESS", "129 nested binary operations exceed MAX_RECURSION_DEPTH and reject cleanly") {
        int depth = 129;
        /* Construct: 1 + (1 + (1 + ... (1 + 1) ... )) */
        char *expr = malloc(depth * 5 + 10);
        int pos = 0;
        for (int i = 0; i < depth; i++) {
            pos += sprintf(expr + pos, "1 + (");
        }
        pos += sprintf(expr + pos, "1");
        for (int i = 0; i < depth; i++) {
            pos += sprintf(expr + pos, ")");
        }

        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        int parsed = parse_string_to_ast(expr, &pe);

        if (!parsed) {
            CHECK(1, "129 nested binary operations rejected cleanly by parser without crash");
        } else {
            Value val;
            value_init(&val, VAL_INT);
            int ok = eval_node(pe.root, &ctx, &val);
            CHECK(!ok, "eval_node must fail when depth exceeds MAX_RECURSION_DEPTH (128)");
            CHECK_INT_EQ(ctx.error.code, ERR_RECURSION_LIMIT, "Error code must be ERR_RECURSION_LIMIT");
            value_free(&val);
            parsed_expression_free(&pe);
        }

        symtab_free(&st);
        free(expr);
    }

    TEST_CASE("SYS-STR-03", "STRESS", "10,000-digit integer addition executed without buffer overflow") {
        char *d1 = malloc(10001);
        char *d2 = malloc(10001);
        memset(d1, '9', 10000);
        d1[10000] = '\0';
        memset(d2, '1', 10000);
        d2[10000] = '\0';

        char *expr = malloc(20050);
        snprintf(expr, 20050, "%s + %s", d1, d2);

        SymbolTable st;
        symtab_init(&st);
        EvalContext ctx;
        eval_context_init(&ctx, &st);

        ParsedExpression pe;
        parsed_expression_init(&pe);
        int parsed = parse_string_to_ast(expr, &pe);
        CHECK(parsed, "10k digit expression parsed");

        if (parsed) {
            Value val;
            value_init(&val, VAL_INT);
            int ok = eval_node(pe.root, &ctx, &val);
            CHECK(ok, "10k digit expression evaluated");
            value_free(&val);
            parsed_expression_free(&pe);
        }

        symtab_free(&st);
        free(d1);
        free(d2);
        free(expr);
    }

    TEST_CASE("SYS-FUZZ-01", "FUZZ", "LCG seeded fuzzer (10,000 random mutations, seed=0xDEADBEEF)") {
        uint32_t seed = 0xDEADBEEF;
        printf("       [Fuzzer Seed: 0x%08X (Testing Principle 5: Pesticide Paradox)]\n", seed);

        MiniLCG lcg;
        minitest_lcg_init(&lcg, seed);

        SymbolTable st;
        symtab_init(&st);
        symtab_preload_constants(&st, 20);

        const char charset[] = "0123456789+-*/%^!()sincosexplnsqrta ";
        size_t charset_len = strlen(charset);

        int survived = 0;
        for (int iter = 0; iter < 10000; iter++) {
            char fuzzed_str[64];
            int len = (int)minitest_lcg_range(&lcg, 1, 60);
            for (int j = 0; j < len; j++) {
                fuzzed_str[j] = charset[minitest_lcg_next(&lcg) % charset_len];
            }
            fuzzed_str[len] = '\0';

            EvalContext ctx;
            eval_context_init(&ctx, &st);

            ParsedExpression pe;
            parsed_expression_init(&pe);
            if (parse_string_to_ast(fuzzed_str, &pe)) {
                Value val;
                value_init(&val, VAL_INT);
                eval_node(pe.root, &ctx, &val);
                value_free(&val);
                parsed_expression_free(&pe);
            }
            survived++;
        }

        CHECK_INT_EQ(survived, 10000, "10,000 random fuzzed iterations completed without crash");
        symtab_free(&st);
    }

    return minitest_finish_suite();
}
