/**
 * ============================================================================
 * File: test_wb_lexer.c
 * Module: Worklet 1 (lexer.c)
 * White-Box Testing Techniques Covered:
 *   - Character validation & pre-scanning
 *   - Balanced parentheses depth & stack tracking
 *   - Token classification: TK_NUMBER, TK_IDENT, TK_OP, TK_LPAREN, TK_RPAREN, TK_COMMA, TK_END
 *   - Component-level table test mirroring syllabus Account Number example
 *   - Column position tracking for error reporting
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "lexer.h"

int main(void) {
    minitest_init_suite("White-Box Unit: Lexer & Tokenizer (Worklet 1)", "tests/test_results.csv");

    TEST_CASE("UT-LEX-01", "UNIT", "Parentheses balancing pre-validator") {
        size_t err_pos = 0;

        /* Balanced expressions */
        CHECK(validate_expression("((1 + 2) * (3 - 4))", &err_pos), "Balanced parens must pass");

        /* Unclosed parenthesis '(' */
        CHECK_INT_EQ(validate_expression("(1 + 2", &err_pos), 0, "Unclosed '(' must fail");
        CHECK_INT_EQ(err_pos, 1, "Error pos at column 1");

        /* Unexpected closing parenthesis ')' */
        CHECK_INT_EQ(validate_expression("1 + 2)", &err_pos), 0, "Extra ')' must fail");
        CHECK_INT_EQ(err_pos, 6, "Error pos at column 6");

        /* Inverted parentheses ')(' */
        CHECK_INT_EQ(validate_expression(")(", &err_pos), 0, "Inverted parens must fail");
        CHECK_INT_EQ(err_pos, 1, "Error pos at column 1");
    }

    TEST_CASE("UT-LEX-02", "UNIT", "Disallowed character detection in pre-validator") {
        size_t err_pos = 0;
        const char *disallowed = "@#$[]{}~";
        for (size_t i = 0; i < strlen(disallowed); i++) {
            char expr[16];
            snprintf(expr, sizeof(expr), "10 %c 5", disallowed[i]);
            CHECK_INT_EQ(validate_expression(expr, &err_pos), 0, "Disallowed character must fail pre-scan");
            CHECK_INT_EQ(err_pos, 4, "Error column must point to character");
        }
    }

    TEST_CASE("UT-LEX-03", "UNIT", "Tokenization sequence: numbers, operators, identifiers, parens") {
        TokenList tl;
        token_list_init(&tl);
        ParseError err;

        const char *expr = "sin(3.1415) + 1e2 * (40 / 8)";
        CHECK(lex_tokenize(expr, &tl, &err), "Tokenize valid expression");

        /* Expected tokens:
         * 0: TK_IDENT  "sin"
         * 1: TK_LPAREN "("
         * 2: TK_NUMBER "3.1415"
         * 3: TK_RPAREN ")"
         * 4: TK_OP     "+"
         * 5: TK_NUMBER "1e2"
         * 6: TK_OP     "*"
         * 7: TK_LPAREN "("
         * 8: TK_NUMBER "40"
         * 9: TK_OP     "/"
         * 10: TK_NUMBER "8"
         * 11: TK_RPAREN ")"
         * 12: TK_END    ""
         */
        CHECK_INT_EQ(tl.count, 13, "Total token count including TK_END");
        CHECK_INT_EQ(tl.items[0].type, TK_IDENT, "Token 0 type");
        CHECK_STR_EQ(tl.items[0].text, "sin", "Token 0 text");
        CHECK_INT_EQ(tl.items[2].type, TK_NUMBER, "Token 2 type");
        CHECK_STR_EQ(tl.items[2].text, "3.1415", "Token 2 text");
        CHECK_INT_EQ(tl.items[5].type, TK_NUMBER, "Token 5 scientific text");
        CHECK_STR_EQ(tl.items[5].text, "1e2", "Token 5 text");
        CHECK_INT_EQ(tl.items[12].type, TK_END, "Sentinel TK_END");

        token_list_free(&tl);
    }

    TEST_CASE("UT-LEX-04", "UNIT", "Mirror syllabus Account Number table: Number-Token component testing") {
        struct {
            const char *input;
            bool valid;
            const char *desc;
        } cases[] = {
            { "1234", true, "Standard integer" },
            { "3.1415", true, "Standard decimal" },
            { ".5", true, "Leading dot decimal" },
            { "1e-5", true, "Scientific notation with minus" },
            { "1E+9", true, "Scientific notation uppercase with plus" },
            { "@123", false, "Disallowed character @" },
            { NULL, false, "NULL input" }
        };
        size_t n = sizeof(cases) / sizeof(cases[0]);

        for (size_t i = 0; i < n; i++) {
            TokenList tl;
            token_list_init(&tl);
            ParseError err;
            bool ok = lex_tokenize(cases[i].input, &tl, &err);

            if (cases[i].valid) {
                CHECK(ok, cases[i].desc);
            } else {
                CHECK(!ok, cases[i].desc);
            }
            token_list_free(&tl);
        }
    }

    return minitest_finish_suite();
}
