#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// File: parser.c
// Purpose: Recursive-descent parser building the Abstract Syntax Tree (AST).
// Role:
//   Enforces mathematical operator precedence and associativity:
//     Level 1 (Lowest):  '+', '-' (Binary addition and subtraction, left-associative)
//     Level 2:           '*', '/', '%' (Multiplication, division, modulo, left-associative)
//     Level 3:           '^' (Exponentiation, right-associative)
//     Level 4:           '+', '-' (Unary prefix signs, right-associative)
//     Level 5:           '!' (Unary postfix factorial)
//     Level 6 (Highest): Primary (Numbers, variables, function calls, grouped parentheses)
// ============================================================================

// Catalog of mathematical functions registered with their arity constraints
const FunctionDef FUNCTIONS[] = {
    { "sin",  1, 1 }, // Sine: 1 argument
    { "cos",  1, 1 }, // Cosine: 1 argument
    { "exp",  1, 1 }, // Exponential: 1 argument
    { "ln",   1, 1 }, // Natural logarithm: 1 argument
    { "sqrt", 1, 1 }, // Square root: 1 argument
    { "abs",  1, 1 }, // Absolute value: 1 argument
    { "gcd",  2, 2 }, // Greatest common divisor: 2 arguments
    { "pow",  2, 2 }, // Power: 2 arguments
    { "mod",  2, 2 }, // Modulo: 2 arguments
    { "fact", 1, 1 }  // Factorial: 1 argument
};

// Count of functions defined in the global catalog
const size_t FUNCTION_COUNT = sizeof(FUNCTIONS) / sizeof(FUNCTIONS[0]);

// Linear search to match function name against registered catalog
// Returns index into FUNCTIONS array, or -1 if unrecognized
int func_lookup(const char *name) {
    if (!name) return -1;
    for (size_t i = 0; i < FUNCTION_COUNT; i++) {
        if (strcmp(name, FUNCTIONS[i].name) == 0) {
            return (int)i;
        }
    }
    return -1; // Not found
}

// ----------------------------------------------------------------------------
// Parser Internal Context & Forward Declarations
// ----------------------------------------------------------------------------

// Internal parser state tracking current token stream position and error destination
typedef struct {
    const TokenList *tokens; // Read-only pointer to token list
    size_t           cursor; // Current token index in tokens->items
    ParseError      *error;  // Destination for error diagnostics
} Parser;

// Grammar hierarchy forward declarations
static ExprNode *parse_expression(Parser *p);
static ExprNode *parse_term(Parser *p);
static ExprNode *parse_power(Parser *p);
static ExprNode *parse_unary(Parser *p);
static ExprNode *parse_postfix(Parser *p);
static ExprNode *parse_primary(Parser *p);

// Retrieves the token currently pointed to by the cursor
static const Token *current_token(Parser *p) {
    if (p->cursor < p->tokens->count)
        return &p->tokens->items[p->cursor];

    // Return the last token (usually TK_END) if past the end
    return &p->tokens->items[p->tokens->count - 1];
}

// Looks ahead one token without advancing the parser cursor
static const Token *peek_next_token(Parser *p) {
    if (p->cursor + 1 < p->tokens->count)
        return &p->tokens->items[p->cursor + 1];

    return NULL; // At or past end of stream
}

// Advances the cursor forward by one token
static void advance(Parser *p) {
    if (p->cursor < p->tokens->count)
        p->cursor++;
}

// Checks if the current token is an operator matching the specified string
static int match_op(Parser *p, const char *op_str) {
    const Token *t = current_token(p);

    if (t->type == TK_OP && strcmp(t->text, op_str) == 0)
        return 1;

    return 0;
}

// Checks if the current token matches the specified TokenType
static int match_type(Parser *p, TokenType type) {
    if (current_token(p)->type == type)
        return 1;

    return 0;
}

// Records an error code, source position, and descriptive message into parser error state
static void set_error(Parser *p, int code, size_t pos, const char *msg) {
    if (p->error) {
        p->error->code = code;
        p->error->position = pos;
        strncpy(p->error->message, msg, sizeof(p->error->message) - 1);
        p->error->message[sizeof(p->error->message) - 1] = '\0';
    }
}

// ----------------------------------------------------------------------------
// Recursive-Descent Grammar Implementation
// ----------------------------------------------------------------------------

// Level 1: expression -> term (( '+' | '-' ) term)*
// Handles lowest precedence left-associative binary addition and subtraction
static ExprNode *parse_expression(Parser *p) {
    // Parse left operand term
    ExprNode *left = parse_term(p);
    if (!left) return NULL;

    // Loop while next token is '+' or '-'
    while (match_op(p, "+") || match_op(p, "-")) {
        // Identify operator opcode
        OpCode op;
        if (strcmp(current_token(p)->text, "+") == 0) {
            op = OP_ADD;
        } else {
            op = OP_SUB;
        }

        size_t op_pos = current_token(p)->pos;
        advance(p); // Consume operator token

        // Parse right operand term
        ExprNode *right = parse_term(p);
        if (!right) {
            // Memory cleanup on failure
            free_expr_node(left);
            return NULL;
        }

        // Fold left and right into a new binary node (left-associative tree growth)
        left = node_new_binary(op, left, right, op_pos);
        if (!left) return NULL;
    }

    return left;
}

// Level 2: term -> power (( '*' | '/' | '%' ) power)*
// Handles left-associative multiplication, division, and modulo
static ExprNode *parse_term(Parser *p) {
    // Parse higher precedence power expression
    ExprNode *left = parse_power(p);
    if (!left) return NULL;

    // Loop while next token is '*', '/', or '%'
    while (match_op(p, "*") || match_op(p, "/") || match_op(p, "%")) {
        OpCode op;

        if (strcmp(current_token(p)->text, "*") == 0) op = OP_MUL;
        else if (strcmp(current_token(p)->text, "/") == 0) op = OP_DIV;
        else op = OP_MOD;

        size_t op_pos = current_token(p)->pos;
        advance(p); // Consume operator token

        // Parse right operand
        ExprNode *right = parse_power(p);
        if (!right) {
            free_expr_node(left);
            return NULL;
        }

        // Fold into binary node
        left = node_new_binary(op, left, right, op_pos);
        if (!left) return NULL;
    }

    return left;
}

// Level 3: power -> unary ( '^' power )?
// Handles exponentiation with right-associativity (e.g. 2^3^2 = 2^(3^2))
static ExprNode *parse_power(Parser *p) {
    // Parse base operand from unary level
    ExprNode *left = parse_unary(p);
    if (!left) return NULL;

    // If '^' is encountered, right-recursive call enforces right-associativity
    if (match_op(p, "^")) {
        size_t op_pos = current_token(p)->pos;
        advance(p); // Consume '^'

        // Right-recursive call: evaluates exponent first
        ExprNode *right = parse_power(p);
        if (!right) {
            free_expr_node(left);
            return NULL;
        }

        return node_new_binary(OP_POW, left, right, op_pos);
    }

    return left;
}

// Level 4: unary -> ( '+' | '-' ) unary | postfix
// Handles prefix unary signs (+x, -x) with positional disambiguation
static ExprNode *parse_unary(Parser *p) {
    // Check if current token is a unary prefix operator
    if (match_op(p, "+") || match_op(p, "-")) {
        OpCode op;
        if (strcmp(current_token(p)->text, "+") == 0) {
            op = OP_POS; // Unary positive
        } else {
            op = OP_NEG; // Unary negation
        }

        size_t op_pos = current_token(p)->pos;
        advance(p); // Consume sign operator

        // Recursive call handles chained unary operators like - - 5
        ExprNode *operand = parse_unary(p);
        if (!operand) return NULL;

        return node_new_unary(op, operand, op_pos);
    }

    // No prefix sign -> Proceed to postfix operations
    return parse_postfix(p);
}

// Level 5: postfix -> primary ( '!' )*
// Handles unary postfix factorial operator (e.g., 5!, 5!!...)
static ExprNode *parse_postfix(Parser *p) {
    // Parse base primary expression
    ExprNode *node = parse_primary(p);
    if (!node) return NULL;

    // Consume all consecutive trailing '!' tokens
    while (match_op(p, "!")) {
        size_t op_pos = current_token(p)->pos;
        advance(p); // Consume '!'

        // Wrap current node in unary factorial node
        node = node_new_unary(OP_FACT, node, op_pos);
        if (!node) return NULL;
    }
    return node;
}

// Level 6: primary -> NUMBER | IDENT | IDENT '(' arglist ')' | '(' expression ')'
// Handles highest precedence atomic expressions
static ExprNode *parse_primary(Parser *p) {
    const Token *tok = current_token(p);

    // Case 1: Numeric Literal (TK_NUMBER)
    if (match_type(p, TK_NUMBER)) {
        ExprNode *node = node_new_number(tok->text, tok->pos);
        if (!node) {
            set_error(p, ERR_INVALID_NUMBER, tok->pos, "Failed to parse number literal");
            return NULL;
        }
        advance(p); // Consume number token
        return node;
    }

    // Case 2: Identifier (Function Call or Variable Reference)
    if (match_type(p, TK_IDENT)) {
        char *name = tok->text;
        size_t name_pos = tok->pos;
        const Token *next = peek_next_token(p);

        // Check if identifier is immediately followed by '(' -> Function Call
        if (next && next->type == TK_LPAREN) {
            // Verify function exists in the FUNCTIONS catalog
            int fn_id = func_lookup(name);
            if (fn_id < 0) {
                set_error(p, 10, name_pos, "Unknown function name");
                return NULL;
            }

            advance(p); // Consume function identifier
            advance(p); // Consume opening '('

            // Parse comma-separated argument list
            size_t argc = 0;
            size_t arg_cap = 4;
            ExprNode **args = (ExprNode **)malloc(arg_cap * sizeof(ExprNode *));
            if (!args) return NULL;

            // If not immediately closed by ')', parse expressions
            if (!match_type(p, TK_RPAREN)) {
                while (1) {
                    ExprNode *arg = parse_expression(p);
                    if (!arg) {
                        // Cleanup allocated arguments on failure
                        for (size_t i = 0; i < argc; i++) free_expr_node(args[i]);
                        free(args);
                        return NULL;
                    }

                    // Dynamically grow arguments array if needed
                    if (argc >= arg_cap) {
                        arg_cap *= 2;
                        ExprNode **new_args = (ExprNode **)realloc(args, arg_cap * sizeof(ExprNode *));
                        if (!new_args) {
                            for (size_t i = 0; i < argc; i++) free_expr_node(args[i]);
                            free_expr_node(arg);
                            free(args);
                            return NULL;
                        }
                        args = new_args;
                    }
                    args[argc++] = arg;

                    // If comma follows, consume it and continue parsing arguments
                    if (match_type(p, TK_COMMA)) {
                        advance(p); // Consume ','
                    } else {
                        break; // No comma -> Finished parsing arguments
                    }
                }
            }

            // Expect matching ')' closing the argument list
            if (!match_type(p, TK_RPAREN)) {
                set_error(p, ERR_UNBALANCED_PAREN, current_token(p)->pos, "Expected ')' closing argument list");
                for (size_t i = 0; i < argc; i++) free_expr_node(args[i]);
                free(args);
                return NULL;
            }
            advance(p); // Consume ')'

            // Validate arity (argument count) at parse time against function specification
            if ((int)argc < FUNCTIONS[fn_id].min_args || (int)argc > FUNCTIONS[fn_id].max_args) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Function '%s' takes %d argument(s), but %zu given",
                         FUNCTIONS[fn_id].name, FUNCTIONS[fn_id].min_args, argc);
                set_error(p, 11, name_pos, msg);
                for (size_t i = 0; i < argc; i++) free_expr_node(args[i]);
                free(args);
                return NULL;
            }

            // Construct function call AST node
            return node_new_call(fn_id, args, argc, name_pos);
        }

        // Not followed by '(' -> Variable name reference
        ExprNode *node = node_new_var(name, name_pos);
        advance(p); // Consume variable identifier token
        return node;
    }

    // Case 3: Grouped Parenthesized Expression: '(' expression ')'
    if (match_type(p, TK_LPAREN)) {
        size_t lparen_pos = tok->pos;
        advance(p); // Consume '('

        // Parse nested sub-expression
        ExprNode *inner = parse_expression(p);
        if (!inner) return NULL;

        // Expect matching ')'
        if (!match_type(p, TK_RPAREN)) {
            set_error(p, ERR_UNBALANCED_PAREN, lparen_pos, "Unclosed '(' opened at this column");
            free_expr_node(inner);
            return NULL;
        }
        advance(p); // Consume ')'

        return inner;
    }

    // Case 4: Unexpected Token encountered
    set_error(p, 12, tok->pos, "Unexpected token or missing operand");
    return NULL;
}

// ----------------------------------------------------------------------------
// Parsed Expression Lifecycle & Entry Points
// ----------------------------------------------------------------------------

// Initializes an empty ParsedExpression container
void parsed_expression_init(ParsedExpression *pe) {
    if (!pe) return;
    pe->source = NULL;
    token_list_init(&pe->tokens);
    pe->root = NULL;
    pe->error.code = ERR_OK;
    pe->error.position = 0;
    pe->error.message[0] = '\0';
}

// Frees all resources held by ParsedExpression (source, token list, AST)
void parsed_expression_free(ParsedExpression *pe) {
    if (!pe) return;
    if (pe->source) {
        free(pe->source);
        pe->source = NULL;
    }
    token_list_free(&pe->tokens);
    if (pe->root) {
        free_expr_node(pe->root);
        pe->root = NULL;
    }
}

// Builds the AST tree from a pre-existing token list
int parse_expression_tree(const TokenList *t, ParsedExpression *out) {
    if (!t || !out) return 0;
    out->root = NULL;

    // Set up parser context
    Parser parser;
    parser.tokens = t;
    parser.cursor = 0;
    parser.error = &out->error;

    // Check for empty token stream
    if (t->count == 0 || (t->count == 1 && t->items[0].type == TK_END)) {
        set_error(&parser, ERR_EMPTY_EXPR, 1, "Empty expression");
        return 0;
    }

    // Parse the root expression
    ExprNode *root = parse_expression(&parser);
    if (!root) {
        return 0; // Error already set by sub-parser
    }

    // Ensure the entire token stream was consumed up to the TK_END sentinel
    if (current_token(&parser)->type != TK_END) {
        set_error(&parser, 13, current_token(&parser)->pos, "Unexpected extra token after valid expression");
        free_expr_node(root);
        return 0;
    }

    out->root = root;
    return 1; // Parse succeeded
}

// End-to-end entry point: pre-validates, tokenizes, and parses raw text into an AST
int parse_string_to_ast(const char *input, ParsedExpression *out) {
    if (!out) return 0;
    parsed_expression_init(out);

    if (!input) {
        out->error.code = ERR_EMPTY_EXPR;
        out->error.position = 1;
        snprintf(out->error.message, sizeof(out->error.message), "Empty input");
        return 0;
    }

    // Save copy of source string
    out->source = strdup(input);

    // Step 1: Pre-validate expression characters and parentheses
    size_t err_pos = 0;
    if (!validate_expression(input, &err_pos)) {
        out->error.code = ERR_UNBALANCED_PAREN;
        out->error.position = err_pos;
        snprintf(out->error.message, sizeof(out->error.message),
                 "Invalid syntax or unbalanced parenthesis at col %zu", err_pos);
        return 0;
    }

    // Step 2: Tokenize string into TokenList
    if (!lex_tokenize(input, &out->tokens, &out->error)) {
        return 0; // Lexical error encountered
    }

    // Step 3: Parse token stream into Abstract Syntax Tree
    if (!parse_expression_tree(&out->tokens, out)) {
        return 0; // Syntax parse error encountered
    }

    return 1; // AST successfully constructed
}



