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
    ExprNode *node = (ExprNode *)calloc(1, sizeof(ExprNode));
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
    ExprNode *node = (ExprNode *)calloc(1, sizeof(ExprNode));
    if (!node) return NULL;

    node->type = NODE_UNARY;
    node->pos = pos;
    node->as.un.op = op;
    node->as.un.operand = operand;
    return node;
}

