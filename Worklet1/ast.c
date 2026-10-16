#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>

ExprNode *node_new_number(const char *str, size_t pos) {
    if (!str) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_NUMBER;
    node->pos = pos;

    Value *val = &node->as.number;

    if (strpbrk(str, ".eE")) {
        val->type = VAL_DEC;
        decimal_init(&val->as.d);
        if (!decimal_from_string(&val->as.d, str)) {
            decimal_free(&val->as.d);
            free(node);
            return NULL;
        }
    } else {
        val->type = VAL_INT;
        bigint_init(&val->as.i);
        if (!bigint_from_string(&val->as.i, str)) {
            bigint_free(&val->as.i);
            free(node);
            return NULL;
        }
    }

    return node;
}

ExprNode *node_new_var(const char *name, size_t pos) {
    if (!name) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_VAR;
    node->pos = pos;
    node->as.var_name = strdup(name);
    if (!node->as.var_name) {
        free(node);
        return NULL;
    }

    return node;
}

ExprNode *node_new_binary(OpCode op, ExprNode *left, ExprNode *right, size_t pos) {
    if (!left || !right) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_BINARY;
    node->pos = pos;
    node->as.bin.op = op;
    node->as.bin.left = left;
    node->as.bin.right = right;
    return node;
}

ExprNode *node_new_unary(OpCode op, ExprNode *operand, size_t pos) {
    if (!operand) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_UNARY;
    node->pos = pos;
    node->as.un.op = op;
    node->as.un.operand = operand;
    return node;
}


ExprNode *node_new_call(int fn_id, ExprNode **args, size_t argc, size_t pos) {
    if (argc > 0 && !args) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_CALL;
    node->pos = pos;
    node->as.call.fn_id = fn_id;
    node->as.call.argc = argc;
    node->as.call.args = args;

    return node;
}


void free_expr_node(ExprNode *root) {
    if (!root) return;

    switch (root->type) {
        case NODE_NUMBER:
            switch (root->as.number.type) {
                case VAL_INT:     bigint_free(&root->as.number.as.i);   break;
                case VAL_DEC:     decimal_free(&root->as.number.as.d);  break;
                case VAL_COMPLEX: complex_free(&root->as.number.as.c);  break;
            }
            break;

        case NODE_VAR:
            free(root->as.var_name);
            break;

        case NODE_BINARY:
            free_expr_node(root->as.bin.left);
            free_expr_node(root->as.bin.right);
            break;

        case NODE_UNARY:
            free_expr_node(root->as.un.operand);
            break;

        case NODE_CALL:
            if (root->as.call.args) {
                for (size_t i = 0; i < root->as.call.argc; i++) {
                    free_expr_node(root->as.call.args[i]);
                }
                free(root->as.call.args);
            }
            break;
    }

    free(root);
}