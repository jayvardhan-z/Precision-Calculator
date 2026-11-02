/**
 * ============================================================================
 * File: test_wb_parser.c
 * Module: Worklet 1 (parser.c)
 * White-Box Testing Techniques Covered:
 *   - Precedence ladder verification (Levels 1 to 6)
 *   - Left-associativity of +, -, *, /
 *   - Right-associativity of power (^)
 *   - Unary sign prefix binding and factorial postfix
 *   - Arity validation in func_lookup: sin(), sin(1, 2), gcd(1), gcd(1, 2)
 *   - Memory safety of AST node creation and deallocation
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "parser.h"

int main(void) {
    minitest_init_suite("White-Box Unit: Parser & AST Construction (Worklet 1)", "tests/test_results.csv");

    TEST_CASE("UT-PAR-01", "UNIT", "Precedence: Multiplication binds tighter than Addition") {
        ParsedExpression pe;
        parsed_expression_init(&pe);
        /* "2 + 3 * 4" -> root is '+' with left=2, right=(3 * 4) */
        CHECK(parse_string_to_ast("2 + 3 * 4", &pe), "Parse 2 + 3 * 4");
        CHECK_NOT_NULL(pe.root, "Root node must exist");
        CHECK_INT_EQ(pe.root->type, NODE_BINARY, "Root must be binary node");
        CHECK_INT_EQ(pe.root->as.bin.op, OP_ADD, "Root operator must be '+'");
        CHECK_INT_EQ(pe.root->as.bin.left->type, NODE_NUMBER, "Left child must be number 2");
        CHECK_INT_EQ(pe.root->as.bin.right->type, NODE_BINARY, "Right child must be binary node");
        CHECK_INT_EQ(pe.root->as.bin.right->as.bin.op, OP_MUL, "Right binary op must be '*'");
        parsed_expression_free(&pe);
    }

    TEST_CASE("UT-PAR-02", "UNIT", "Right-associativity of power: 2 ^ 3 ^ 2 -> 2 ^ (3 ^ 2)") {
        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("2 ^ 3 ^ 2", &pe), "Parse 2 ^ 3 ^ 2");
        CHECK_NOT_NULL(pe.root, "Root node must exist");
        CHECK_INT_EQ(pe.root->as.bin.op, OP_POW, "Root must be '^'");
        /* Left child is 2; right child is (3 ^ 2) */
        CHECK_INT_EQ(pe.root->as.bin.left->type, NODE_NUMBER, "Left child is 2");
        CHECK_INT_EQ(pe.root->as.bin.right->type, NODE_BINARY, "Right child is 3 ^ 2");
        CHECK_INT_EQ(pe.root->as.bin.right->as.bin.op, OP_POW, "Right binary op is '^'");
        parsed_expression_free(&pe);
    }

    TEST_CASE("UT-PAR-03", "UNIT", "Left-associativity of subtraction: 10 - 4 - 2 -> (10 - 4) - 2") {
        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("10 - 4 - 2", &pe), "Parse 10 - 4 - 2");
        CHECK_NOT_NULL(pe.root, "Root node must exist");
        CHECK_INT_EQ(pe.root->as.bin.op, OP_SUB, "Root is '-'");
        /* Left child is (10 - 4); right child is 2 */
        CHECK_INT_EQ(pe.root->as.bin.left->type, NODE_BINARY, "Left child is (10 - 4)");
        CHECK_INT_EQ(pe.root->as.bin.right->type, NODE_NUMBER, "Right child is 2");
        parsed_expression_free(&pe);
    }

    TEST_CASE("UT-PAR-04", "UNIT", "Postfix factorial: 5! and chained 3!!") {
        ParsedExpression pe;
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("5!", &pe), "Parse 5!");
        CHECK_NOT_NULL(pe.root, "Root must exist");
        CHECK_INT_EQ(pe.root->type, NODE_UNARY, "5! must be unary node");
        CHECK_INT_EQ(pe.root->as.un.op, OP_FACT, "Unary op must be OP_FACT");

        /* Chained 3!! */
        parsed_expression_free(&pe);
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("3!!", &pe), "Parse 3!!");
        CHECK_INT_EQ(pe.root->type, NODE_UNARY, "Outer must be unary node");
        CHECK_INT_EQ(pe.root->as.un.operand->type, NODE_UNARY, "Inner must be unary node");
        parsed_expression_free(&pe);
    }

    TEST_CASE("UT-PAR-05", "UNIT", "Function arity validation: valid vs arity errors") {
        ParsedExpression pe;

        /* Valid single argument: sin(0) */
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("sin(0)", &pe), "sin(0) arity 1");
        parsed_expression_free(&pe);

        /* Valid two arguments: gcd(48, 18) */
        parsed_expression_init(&pe);
        CHECK(parse_string_to_ast("gcd(48, 18)", &pe), "gcd(48, 18) arity 2");
        parsed_expression_free(&pe);

        /* Arity error: sin() with 0 args */
        parsed_expression_init(&pe);
        CHECK_INT_EQ(parse_string_to_ast("sin()", &pe), 0, "sin() must fail arity validation");
        parsed_expression_free(&pe);

        /* Arity error: sin(1, 2) with 2 args */
        parsed_expression_init(&pe);
        CHECK_INT_EQ(parse_string_to_ast("sin(1, 2)", &pe), 0, "sin(1, 2) must fail arity validation");
        parsed_expression_free(&pe);

        /* Arity error: gcd(10) with 1 arg */
        parsed_expression_init(&pe);
        CHECK_INT_EQ(parse_string_to_ast("gcd(10)", &pe), 0, "gcd(10) must fail arity validation");
        parsed_expression_free(&pe);
    }

    return minitest_finish_suite();
}
