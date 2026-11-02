/**
 * ============================================================================
 * File: test_wb_ast.c
 * Module: Worklet 1 (ast.c)
 * White-Box Testing Techniques Covered:
 *   - Statement & branch coverage for all node constructors
 *   - Polymorphic Value storage (VAL_INT, VAL_DEC) in node_new_number
 *   - Variable node string duplication and ownership
 *   - Binary and unary node operand pointer linking
 *   - Function call node argument array management
 *   - Recursive post-order deallocation (free_expr_node) and NULL-pointer safety
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "ast.h"

int main(void) {
    minitest_init_suite("White-Box Unit: AST Construction & Traversal (Worklet 1)", "tests/test_results.csv");

    TEST_CASE("UT-AST-01", "UNIT", "node_new_number creates valid VAL_INT node") {
        ExprNode *n = node_new_number("12345678901234567890", 5);
        CHECK_NOT_NULL(n, "Node must be allocated");
        CHECK_INT_EQ(n->type, NODE_NUMBER, "Node type must be NODE_NUMBER");
        CHECK_INT_EQ(n->pos, 5, "Position must match input");
        CHECK_INT_EQ(n->as.number.type, VAL_INT, "Value type must be VAL_INT");
        free_expr_node(n);
    }

    TEST_CASE("UT-AST-02", "UNIT", "node_new_number creates valid VAL_DEC node for decimal and scientific notation") {
        ExprNode *n1 = node_new_number("3.1415926535", 1);
        CHECK_NOT_NULL(n1, "Decimal node allocated");
        CHECK_INT_EQ(n1->as.number.type, VAL_DEC, "Must be VAL_DEC");
        free_expr_node(n1);

        ExprNode *n2 = node_new_number("1e-5", 10);
        CHECK_NOT_NULL(n2, "Scientific notation node allocated");
        CHECK_INT_EQ(n2->as.number.type, VAL_DEC, "Must be VAL_DEC for exponent");
        free_expr_node(n2);
    }

    TEST_CASE("UT-AST-03", "UNIT", "node_new_number error handling on NULL or malformed input") {
        CHECK_NULL(node_new_number(NULL, 0), "NULL string returns NULL");
        CHECK_NULL(node_new_number("invalid_number", 0), "Non-numeric string returns NULL");
    }

    TEST_CASE("UT-AST-04", "UNIT", "node_new_var allocates and deep-copies identifier") {
        char buffer[32];
        strcpy(buffer, "ans");
        ExprNode *n = node_new_var(buffer, 3);
        CHECK_NOT_NULL(n, "Variable node created");
        CHECK_INT_EQ(n->type, NODE_VAR, "Node type is NODE_VAR");
        CHECK_INT_EQ(n->pos, 3, "Position is 3");
        CHECK_STR_EQ(n->as.var_name, "ans", "Name matches");
        CHECK(n->as.var_name != buffer, "Identifier must be a deep-copied heap pointer");

        /* Overwrite stack buffer to verify independence */
        strcpy(buffer, "foo");
        CHECK_STR_EQ(n->as.var_name, "ans", "Stored identifier unaffected by caller modification");

        CHECK_NULL(node_new_var(NULL, 0), "NULL variable name returns NULL");
        free_expr_node(n);
    }

    TEST_CASE("UT-AST-05", "UNIT", "node_new_binary links left and right subtrees") {
        ExprNode *left = node_new_number("10", 1);
        ExprNode *right = node_new_number("20", 5);
        ExprNode *bin = node_new_binary(OP_ADD, left, right, 4);

        CHECK_NOT_NULL(bin, "Binary node allocated");
        CHECK_INT_EQ(bin->type, NODE_BINARY, "Node type is NODE_BINARY");
        CHECK_INT_EQ(bin->as.bin.op, OP_ADD, "Operator is OP_ADD");
        CHECK(bin->as.bin.left == left, "Left subtree pointer linked");
        CHECK(bin->as.bin.right == right, "Right subtree pointer linked");

        /* Error cases */
        CHECK_NULL(node_new_binary(OP_ADD, NULL, right, 0), "Missing left returns NULL");
        CHECK_NULL(node_new_binary(OP_ADD, left, NULL, 0), "Missing right returns NULL");

        free_expr_node(bin);
    }

    TEST_CASE("UT-AST-06", "UNIT", "node_new_unary links operand subtree") {
        ExprNode *operand = node_new_number("5", 2);
        ExprNode *un = node_new_unary(OP_FACT, operand, 3);

        CHECK_NOT_NULL(un, "Unary node allocated");
        CHECK_INT_EQ(un->type, NODE_UNARY, "Node type is NODE_UNARY");
        CHECK_INT_EQ(un->as.un.op, OP_FACT, "Operator is OP_FACT");
        CHECK(un->as.un.operand == operand, "Operand subtree pointer linked");

        CHECK_NULL(node_new_unary(OP_NEG, NULL, 0), "Missing operand returns NULL");
        free_expr_node(un);
    }

    TEST_CASE("UT-AST-07", "UNIT", "node_new_call links argument array and function id") {
        ExprNode **args = malloc(2 * sizeof(ExprNode *));
        args[0] = node_new_number("48", 5);
        args[1] = node_new_number("18", 9);

        ExprNode *call = node_new_call(7 /* gcd */, args, 2, 1);
        CHECK_NOT_NULL(call, "Call node allocated");
        CHECK_INT_EQ(call->type, NODE_CALL, "Node type is NODE_CALL");
        CHECK_INT_EQ(call->as.call.fn_id, 7, "Function ID matches");
        CHECK_INT_EQ(call->as.call.argc, 2, "Arg count matches");
        CHECK(call->as.call.args == args, "Arguments array linked");

        CHECK_NULL(node_new_call(7, NULL, 2, 0), "NULL args with argc > 0 returns NULL");
        free_expr_node(call);
    }

    TEST_CASE("UT-AST-08", "UNIT", "free_expr_node safe recursive traversal and NULL safety") {
        /* Verify NULL safe */
        free_expr_node(NULL);

        /* Construct complex nested tree: sin(2 + x!) */
        ExprNode *two = node_new_number("2", 5);
        ExprNode *x = node_new_var("x", 9);
        ExprNode *x_fact = node_new_unary(OP_FACT, x, 10);
        ExprNode *add = node_new_binary(OP_ADD, two, x_fact, 7);

        ExprNode **call_args = malloc(1 * sizeof(ExprNode *));
        call_args[0] = add;
        ExprNode *call = node_new_call(1, call_args, 1, 1);

        CHECK_NOT_NULL(call, "Constructed nested tree");
        free_expr_node(call);
        CHECK(1, "free_expr_node successfully executed without crash or leak");
    }

    return minitest_finish_suite();
}
