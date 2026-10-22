#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

// ============================================================================
// File: parser.h
// Purpose: Recursive-descent parser and AST generator for mathematical expressions.
// Role:
//   Translates a stream of lexical tokens into a hierarchical Abstract Syntax
//   Tree (AST) according to operator precedence and associativity rules.
//   Also provides function lookup and arity validation at parse time.
// ============================================================================

// ----------------------------------------------------------------------------
// Function Definition Table & Lookup
// ----------------------------------------------------------------------------

// Metadata describing supported mathematical functions and required argument counts
typedef struct {
    const char *name;     // Function identifier string (e.g. "sin", "gcd")
    int         min_args; // Minimum required arguments
    int         max_args; // Maximum permitted arguments
} FunctionDef;

// Global catalog of supported functions
extern const FunctionDef FUNCTIONS[];
// Number of functions registered in FUNCTIONS catalog
extern const size_t FUNCTION_COUNT;

// Looks up function name in FUNCTIONS table; returns index or -1 if not found
int func_lookup(const char *name);

// ----------------------------------------------------------------------------
// Parsed Expression Container & Parser Entry Points
// ----------------------------------------------------------------------------

// Container bundling raw input source, tokenized stream, AST root, and error details
typedef struct {
    char       *source; // Heap copy of raw input string
    TokenList   tokens; // Token list produced by lexer
    ExprNode   *root;   // Root node of constructed AST
    ParseError  error;  // Diagnostic error information if parsing failed
} ParsedExpression;

// Initializes all fields in a ParsedExpression structure to zero / default state
void parsed_expression_init(ParsedExpression *pe);

// Frees all resources (source string, token list, and entire AST) held by ParsedExpression
void parsed_expression_free(ParsedExpression *pe);

// Parses an already-tokenized TokenList into an AST stored in out->root
// Returns 1 on success, 0 on parse error
int parse_expression_tree(const TokenList *t, ParsedExpression *out);

// High-level pipeline: validates string, tokenizes it, and builds AST in out
// Returns 1 on success, 0 on failure
int parse_string_to_ast(const char *input, ParsedExpression *out);

#endif /* PARSER_H */


