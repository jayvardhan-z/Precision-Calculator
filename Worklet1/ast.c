#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// File: ast.c
// Purpose: Constructors and destructors for Abstract Syntax Tree (AST) nodes.
// Role:
//   Allocates and initializes AST nodes representing numeric constants,
//   variables, unary operations, binary operations, and function calls.
//   Also handles safe, recursive deallocation of the entire AST tree.
// ============================================================================

// Constructs a new AST node representing a numeric constant literal
ExprNode *node_new_number(const char *str, size_t pos) {
    // Return NULL if input string pointer is invalid
    if (!str) return NULL;

    // Allocate memory for the AST node, zeroing all fields
    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    // Set node kind and source column position
    node->type = NODE_NUMBER;
    node->pos = pos;

    Value *val = &node->as.number;

    // Check if literal contains a decimal point or scientific exponent notation (e/E)
    if (strpbrk(str, ".eE")) {
        // Contains fractional or exponent component -> Store as BigDecimal
        val->type = VAL_DEC;
        decimal_init(&val->as.d);

        // Parse string into BigDecimal structure (significand + scale)
        if (!decimal_from_string(&val->as.d, str)) {
            // Cleanup on parse failure
            decimal_free(&val->as.d);
            free(node);
            return NULL;
        }
    } else {
        // Pure integer literal -> Store as BigInteger (base-10^9 limbs)
        val->type = VAL_INT;
        bigint_init(&val->as.i);

        // Parse integer string into BigInteger limbs
        if (!bigint_from_string(&val->as.i, str)) {
            // Cleanup on parse failure
            bigint_free(&val->as.i);
            free(node);
            return NULL;
        }
    }

    return node;
}

// Constructs a new AST node representing a variable identifier (e.g., 'x', 'ans')
ExprNode *node_new_var(const char *name, size_t pos) {
    // Return NULL if variable name is missing
    if (!name) return NULL;

    // Allocate node structure
    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    node->type = NODE_VAR;
    node->pos = pos;

    // Duplicate identifier name string onto heap
    node->as.var_name = strdup(name);
    if (!node->as.var_name) {
        // Memory allocation failure guard
        free(node);
        return NULL;
    }

    return node;
}

// Constructs a new AST node for a binary operation (+, -, *, /, %, ^)
ExprNode *node_new_binary(OpCode op, ExprNode *left, ExprNode *right, size_t pos) {
    // Both left and right operands must be non-NULL
    if (!left || !right) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    // Store binary operator metadata and link left/right child subtrees
    node->type = NODE_BINARY;
    node->pos = pos;
    node->as.bin.op = op;
    node->as.bin.left = left;
    node->as.bin.right = right;

    return node;
}

// Constructs a new AST node for a unary operation (negation, positive sign, factorial)
ExprNode *node_new_unary(OpCode op, ExprNode *operand, size_t pos) {
    // Unary operand child must be non-NULL
    if (!operand) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    // Store unary operator metadata and link target operand subtree
    node->type = NODE_UNARY;
    node->pos = pos;
    node->as.un.op = op;
    node->as.un.operand = operand;

    return node;
}

// Constructs a new AST node representing a function call invocation (e.g., sin(x))
ExprNode *node_new_call(int fn_id, ExprNode **args, size_t argc, size_t pos) {
    // If argument count > 0, argument pointers array must be valid
    if (argc > 0 && !args) return NULL;

    ExprNode *node = calloc(1, sizeof(*node));
    if (!node) return NULL;

    // Store function identifier, argument count, and arguments array pointer
    node->type = NODE_CALL;
    node->pos = pos;
    node->as.call.fn_id = fn_id;
    node->as.call.argc = argc;
    node->as.call.args = args;

    return node;
}

// Recursively walks the AST in post-order and frees all allocated memory
void free_expr_node(ExprNode *root) {
    // Base case: NULL pointer requires no operation
    if (!root) return;

    // Deallocate node-specific payloads depending on the node discriminator
    switch (root->type) {
        case NODE_NUMBER:
            // Release memory associated with number value (limbs or decimals)
            switch (root->as.number.type) {
                case VAL_INT:     bigint_free(&root->as.number.as.i);   break;
                case VAL_DEC:     decimal_free(&root->as.number.as.d);  break;
                case VAL_COMPLEX: complex_free(&root->as.number.as.c);  break;
            }
            break;

        case NODE_VAR:
            // Free the duplicated variable name string
            free(root->as.var_name);
            break;

        case NODE_BINARY:
            // Recursively free left child subtree
            free_expr_node(root->as.bin.left);
            // Recursively free right child subtree
            free_expr_node(root->as.bin.right);
            break;

        case NODE_UNARY:
            // Recursively free operand subtree
            free_expr_node(root->as.un.operand);
            break;

        case NODE_CALL:
            // Recursively free each argument expression subtree
            if (root->as.call.args) {
                for (size_t i = 0; i < root->as.call.argc; i++) {
                    free_expr_node(root->as.call.args[i]);
                }
                // Free the array holding argument pointers
                free(root->as.call.args);
            }
            break;
    }

    // Free the AST node itself
    free(root);
}