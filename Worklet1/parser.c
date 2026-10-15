#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const FunctionDef FUNCTIONS[] = {
    { "sin",  1, 1 },
    { "cos",  1, 1 },
    { "exp",  1, 1 },
    { "ln",   1, 1 },
    { "sqrt", 1, 1 },
    { "abs",  1, 1 },
    { "gcd",  2, 2 },
    { "pow",  2, 2 },
    { "mod",  2, 2 },
    { "fact", 1, 1 }
};

const size_t FUNCTION_COUNT = sizeof(FUNCTIONS) / sizeof(FUNCTIONS[0]);

int func_lookup(const char *name) {
    if (!name) return -1;
    int i = 0;
    for (i = 0; i < FUNCTION_COUNT; i++) {
        if (strcmp(name, FUNCTIONS[i].name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

// Parser Context & Forward Declarations

typedef struct {
    const TokenList *tokens;
    size_t           cursor;
    ParseError      *error;
} Parser;

Token *current_token(Parser *p) {
    if (p->cursor < p->tokens->count)
        return &p->tokens->items[p->cursor];

    return &p->tokens->items[p->tokens->count - 1];
}

Token *peek_next_token(Parser *p) {
    if (p->cursor + 1 < p->tokens->count)
        return &p->tokens->items[p->cursor + 1];

    return NULL;
}

void advance(Parser *p) {
    if (p->cursor < p->tokens->count)
        p->cursor++;
}

int match_op(Parser *p, const char *op_str) {
    const Token *t = current_token(p);

    if (t->type == TK_OP && strcmp(t->text, op_str) == 0)
        return 1;

    return 0;
}

int match_type(Parser *p, TokenType type) {
    if (current_token(p)->type == type)
        return 1;

    return 0;
}

void set_error(Parser *p, int code, size_t pos, const char *msg) {
    if (p->error) {
        p->error->code = code;
        p->error->position = pos;
        strcpy(p->error->message, msg);
    }
}
