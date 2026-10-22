#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ============================================================================
// File: lexer.c
// Purpose: Implementation of lexical analyzer and pre-validation logic.
// Role:
//   Validates raw input strings, enforces balanced parentheses, and scans
//   character sequences into discrete tokens with precise 1-based source
//   position tracking for descriptive error reporting.
// ============================================================================

// Converts a token enum type into a readable string for logging and diagnostics
const char *token_type_name(TokenType type) {
    switch (type) {
        case TK_NUMBER: return "NUMBER";
        case TK_IDENT:  return "IDENT";
        case TK_OP:     return "OP";
        case TK_LPAREN: return "LPAREN";
        case TK_RPAREN: return "RPAREN";
        case TK_COMMA:  return "COMMA";
        case TK_END:    return "END";
        default:        return "UNKNOWN";
    }
}

// ----------------------------------------------------------------------------
// Token List Lifecycle
// ----------------------------------------------------------------------------

// Initializes an empty token list with zero count and NULL buffer
void token_list_init(TokenList *tl) {
    if (!tl) return;
    tl->items = NULL;
    tl->count = 0;
    tl->capacity = 0;
}

// Frees all heap memory held by token strings and the items array
void token_list_free(TokenList *tl) {
    if (!tl) return;
    if (tl->items) {
        // Free dynamically allocated text string of each individual token
        for (size_t i = 0; i < tl->count; i++) {
            if (tl->items[i].text) {
                free(tl->items[i].text);
                tl->items[i].text = NULL;
            }
        }
        // Free the array buffer itself
        free(tl->items);
        tl->items = NULL;
    }
    tl->count = 0;
    tl->capacity = 0;
}

// Appends a new token to the token list, resizing the buffer exponentially if needed
int token_list_add(TokenList *tl, TokenType type, const char *text, size_t pos) {
    if (!tl) return 0;

    // Expand buffer capacity if current count reached capacity
    if (tl->count >= tl->capacity) {
        // Start with 16 slots or double existing capacity
        size_t new_cap = (tl->capacity == 0 ? 16 : tl->capacity * 2);
        Token *new_items = (Token *)realloc(tl->items, new_cap * sizeof(Token));
        if (!new_items) return 0; // Reallocation failure guard
        tl->items = new_items;
        tl->capacity = new_cap;
    }

    // Duplicate token text onto heap
    size_t len = (text ? strlen(text) : 0);
    char *dup = (char *)malloc(len + 1);
    if (!dup) return 0;
    if (text) {
        memcpy(dup, text, len);
    }
    dup[len] = '\0';

    // Store token attributes in the list
    tl->items[tl->count].type = type;
    tl->items[tl->count].text = dup;
    tl->items[tl->count].pos = pos;
    tl->count++;
    return 1;
}

// ----------------------------------------------------------------------------
// Pre-validation
// ----------------------------------------------------------------------------

// Pre-scans the raw expression to check character validity and parentheses balance
// Returns 1 if valid; returns 0 and sets *err_pos to the offending column if invalid
int validate_expression(const char *input, size_t *err_pos) {
    if (err_pos) *err_pos = 0;
    if (!input) return 0;

    // Check for empty or whitespace-only expressions
    const char *p = input;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) {
        if (err_pos) *err_pos = 1;
        return 0; // Reject empty input
    }

    size_t col = 1;
    // Static stack to record the column position of each opened '(' for accurate error carets
    size_t paren_stack[512];
    size_t paren_depth = 0;

    p = input;
    while (*p) {
        char c = *p;

        if (c == '(') {
            // Push column position of '(' onto tracking stack
            if (paren_depth < sizeof(paren_stack) / sizeof(paren_stack[0])) {
                paren_stack[paren_depth] = col;
            }
            paren_depth++;
        } else if (c == ')') {
            // If ')' encountered without matching '(', report error immediately at this column
            if (paren_depth == 0) {
                if (err_pos) *err_pos = col;
                return 0; // Unmatched closing parenthesis
            }
            paren_depth--;
        } else if (!isspace((unsigned char)c) && !isalnum((unsigned char)c) && c != '_') {
            // Check non-alphanumeric punctuation against allowed arithmetic operators
            if (!strchr("+-*/%^!=;,. ", c)) {
                if (err_pos) *err_pos = col;
                return 0; // Disallowed/illegal character found
            }
        }

        p++;
        col++;
    }

    // Check for unclosed opening parentheses remaining on the stack
    if (paren_depth > 0) {
        if (err_pos) {
            // Point error caret to the innermost unclosed '('
            size_t idx = (paren_depth - 1 < sizeof(paren_stack) / sizeof(paren_stack[0]))
                         ? paren_depth - 1 : 0;
            *err_pos = paren_stack[idx];
        }
        return 0; // Unclosed opening parenthesis
    }

    return 1; // Validation passed successfully
}

// ----------------------------------------------------------------------------
// Lexer (Tokenizer)
// ----------------------------------------------------------------------------

// Scans an expression string into a sequence of tokens stored in out
// Returns 1 on success, 0 on failure with err populated
int lex_tokenize(const char *input, TokenList *out, ParseError *err) {
    if (!out) return 0;
    token_list_init(out);

    // Initialize error descriptor
    if (err) {
        err->code = ERR_OK;
        err->position = 0;
        err->message[0] = '\0';
    }

    // Handle NULL input guard
    if (!input) {
        if (err) {
            err->code = ERR_EMPTY_EXPR;
            err->position = 1;
            snprintf(err->message, sizeof(err->message), "Empty expression");
        }
        return 0;
    }

    const char *p = input;
    size_t col = 1; // 1-indexed source column tracking

    while (*p) {
        // Step 1: Skip whitespace characters (spaces, tabs, newlines)
        if (isspace((unsigned char)*p)) {
            p++;
            col++;
            continue;
        }

        // Step 2: Numeric literals (integers, decimals, and scientific notation)
        // Matches: digits followed by optional fractional part, or starting with '.' followed by digits
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)*(p + 1)))) {
            size_t start_col = col;
            const char *start_ptr = p;
            int has_dot = 0;

            // Handle numbers starting with decimal point (e.g., .5)
            if (*p == '.') {
                has_dot = 1;
                p++;
                col++;
            }

            // Consume integer digits
            while (isdigit((unsigned char)*p)) {
                p++;
                col++;
            }

            // Consume fractional digits after decimal point
            if (!has_dot && *p == '.' && isdigit((unsigned char)*(p + 1))) {
                has_dot = 1;
                p++; // Consume '.'
                col++;
                while (isdigit((unsigned char)*p)) {
                    p++;
                    col++;
                }
            }

            // Consume optional scientific notation exponent: [eE][+-]?[0-9]+
            if (*p == 'e' || *p == 'E') {
                const char *exp_ptr = p + 1;
                if (*exp_ptr == '+' || *exp_ptr == '-') {
                    exp_ptr++;
                }
                // Exponent must have at least one digit
                if (isdigit((unsigned char)*exp_ptr)) {
                    p = exp_ptr;
                    col = start_col + (size_t)(p - start_ptr);
                    while (isdigit((unsigned char)*p)) {
                        p++;
                        col++;
                    }
                }
            }

            // Extract numeric literal substring
            size_t len = (size_t)(p - start_ptr);
            char *num_str = (char *)malloc(len + 1);
            if (!num_str) {
                token_list_free(out);
                return 0;
            }
            memcpy(num_str, start_ptr, len);
            num_str[len] = '\0';

            // Add numeric token to list
            if (!token_list_add(out, TK_NUMBER, num_str, start_col)) {
                free(num_str);
                token_list_free(out);
                return 0;
            }
            free(num_str);
            continue;
        }

        // Step 3: Identifiers (functions like sin, cos, gcd or variables like x, ans, pi, e)
        // Pattern: [a-zA-Z_][a-zA-Z0-9_]*
        if (isalpha((unsigned char)*p) || *p == '_') {
            size_t start_col = col;
            const char *start_ptr = p;

            // Consume subsequent alphanumeric characters and underscores
            while (isalnum((unsigned char)*p) || *p == '_') {
                p++;
                col++;
            }

            // Extract identifier substring
            size_t len = (size_t)(p - start_ptr);
            char *ident_str = (char *)malloc(len + 1);
            if (!ident_str) {
                token_list_free(out);
                return 0;
            }
            memcpy(ident_str, start_ptr, len);
            ident_str[len] = '\0';

            // Add identifier token to list
            if (!token_list_add(out, TK_IDENT, ident_str, start_col)) {
                free(ident_str);
                token_list_free(out);
                return 0;
            }
            free(ident_str);
            continue;
        }

        // Step 4: Grouping symbols & Delimiters
        // Opening parenthesis '('
        if (*p == '(') {
            if (!token_list_add(out, TK_LPAREN, "(", col)) {
                token_list_free(out);
                return 0;
            }
            p++;
            col++;
            continue;
        }

        // Closing parenthesis ')'
        if (*p == ')') {
            if (!token_list_add(out, TK_RPAREN, ")", col)) {
                token_list_free(out);
                return 0;
            }
            p++;
            col++;
            continue;
        }

        // Argument separator comma ','
        if (*p == ',') {
            if (!token_list_add(out, TK_COMMA, ",", col)) {
                token_list_free(out);
                return 0;
            }
            p++;
            col++;
            continue;
        }

        // Step 5: Arithmetic & mathematical operators: +, -, *, /, %, ^, !
        if (strchr("+-*/%^!", *p)) {
            char op_str[2] = { *p, '\0' };
            if (!token_list_add(out, TK_OP, op_str, col)) {
                token_list_free(out);
                return 0;
            }
            p++;
            col++;
            continue;
        }

        // Step 6: Illegal / Disallowed Character encountered
        if (err) {
            err->code = ERR_INVALID_CHAR;
            err->position = col;
            snprintf(err->message, sizeof(err->message),
                     "Illegal character '%c' at column %zu", *p, col);
        }
        token_list_free(out);
        return 0;
    }

    // Step 7: Append TK_END sentinel token to demarcate the end of token stream
    if (!token_list_add(out, TK_END, "", col)) {
        token_list_free(out);
        return 0;
    }

    return 1; // Successfully tokenized
}


