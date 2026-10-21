// ============================================================================
// File: Worklet3/eval.h
// Description: Abstract Syntax Tree (AST) evaluator, symbol table management,
//              dynamic value type promotion, and runtime execution context.
//
// Overview:
// The evaluator module bridges the high-level semantic representation produced
// by the parser (ExprNode AST) with the low-level arbitrary-precision numerical
// engines (BigInteger, BigDecimal, Complex). It manages:
// 1. Recursive Post-Order AST Evaluation: Evaluating child nodes first, then
//    computing parent operations with recursion depth protection.
// 2. Dynamic Type Promotion: Automatically lifting numeric types across the
//    hierarchy: BigInteger (VAL_INT) -> BigDecimal (VAL_DEC) -> Complex (VAL_COMPLEX).
// 3. Precision & Angle Context: Propagating decimal rounding precision, angle
//    units (radian vs. degree), and execution limits.
// 4. Symbol Table: Storing and resolving variables and predefined constants
//    (pi, e, i, ans).
// 5. Error Reporting: Providing precise error codes and source code offsets.
// ============================================================================

#ifndef EVAL_H
#define EVAL_H

#include <stddef.h>

// Core AST node definitions and tagged union Value system
#include "ast.h"

// Scientific and transcendental function interfaces
#include "functions.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum permitted AST traversal recursion depth to prevent stack overflow
// on deeply nested or adversarial expressions (e.g., 1+(1+(1+...))).
#define MAX_RECURSION_DEPTH 128

// ============================================================================
// Angular Unit Modes
// ============================================================================

// Controls the interpretation of angles passed to trigonometric functions
// (sin, cos) and the output of inverse trigonometric functions.
typedef enum {
    ANGLE_RADIAN, // Standard radian mode (default: 2*pi radians = 360 deg)
    ANGLE_DEGREE  // Degree mode (converts degrees to radians: rad = deg * pi / 180)
} AngleMode;

// ============================================================================
// Evaluation Error Codes & Diagnostics
// ============================================================================

// Diagnostic error codes identifying evaluation failures
typedef enum {
    ERR_NONE = 0,        // Evaluation succeeded without error
    ERR_DIV_BY_ZERO,     // Integer, decimal, or complex division or modulo by zero
    ERR_UNDEFINED_VAR,   // Variable identifier not found in SymbolTable
    ERR_UNDEFINED_FUNC,  // Function name does not match any registered function
    ERR_INVALID_ARITY,   // Argument count does not match function signature
    ERR_TYPE_MISMATCH,   // Operands incompatible with requested operation
    ERR_DOMAIN,          // Mathematical domain error (e.g. sqrt(-1), ln(0))
    ERR_RECURSION_LIMIT, // AST traversal exceeded MAX_RECURSION_DEPTH
    ERR_OVERFLOW,        // Exponent or factorial index exceeds representable bounds
    ERR_INTERNAL         // Internal calculation or memory allocation error
} EvalErrorCode;

// Detailed error description containing diagnostic code, position, and message
typedef struct {
    EvalErrorCode code;        // Specific failure classification
    size_t        pos;         // Character byte offset in original input string
    char          message[256];// Human-readable error description buffer
} EvalError;

// ============================================================================
// Symbol Table (Variables & Mathematical Constants)
// ============================================================================

// Individual entry in the symbol table binding an identifier to a Value
typedef struct {
    char  *name;  // Dynamically allocated null-terminated identifier string
    Value  value; // Stored numeric value (BigInteger, BigDecimal, or Complex)
} Symbol;

// Dynamic array of variable and constant bindings
typedef struct {
    Symbol *symbols;  // Contiguous heap-allocated array of Symbol entries
    size_t  count;    // Current number of populated symbols
    size_t  capacity; // Total allocated slot capacity in symbols buffer
} SymbolTable;

// Initializes an empty symbol table with zero capacity
void symtab_init(SymbolTable *st);

// Releases all allocated memory associated with symbols and their names
void symtab_free(SymbolTable *st);

// Inserts or updates a symbol binding. If the variable already exists,
// its prior value is freed and replaced.
// Returns 1 on success, 0 on memory allocation failure.
int symtab_set(SymbolTable *st, const char *name, const Value *v);

// Retrieves a copy of the value bound to the specified variable name.
// Returns 1 if found, 0 if undefined.
int symtab_get(const SymbolTable *st, const char *name, Value *out);

// Pre-populates the symbol table with standard mathematical constants:
// - "pi"  : Ratio of circumference to diameter (BigDecimal)
// - "e"   : Euler's number (BigDecimal)
// - "i"   : Imaginary unit 0 + 1i (Complex)
// - "ans" : Result of previous interactive REPL evaluation (initialized to 0)
// Returns 1 on success, 0 on failure.
int symtab_preload_constants(SymbolTable *st, int precision);

// ============================================================================
// Evaluation Context
// ============================================================================

// Global runtime context passed through AST evaluation traversal
typedef struct {
    int          precision;  // Target fractional decimal digits for division/functions
    AngleMode    angle_mode; // Current angular interpretation (ANGLE_RADIAN or ANGLE_DEGREE)
    RoundingMode rounding;   // Active decimal rounding mode (e.g., ROUND_HALF_UP)
    SymbolTable *vars;       // Pointer to active variable / constant symbol table
    int          depth;      // Current AST recursive descent depth counter
    EvalError    error;      // Latched error diagnostic recorded during evaluation
} EvalContext;

// Initializes an evaluation context with default precision (20), radian mode,
// and the provided symbol table reference.
void eval_context_init(EvalContext *ctx, SymbolTable *st);

// ============================================================================
// Value Lifecycle & Type Promotion
// ============================================================================

// Initializes a Value container to default state for the specified type
void value_init(Value *v, ValueType type);

// Frees heap resources allocated within a Value (limbs, strings)
void value_free(Value *v);

// Deep-copies a source Value to a destination Value container.
// Returns 1 on success, 0 on failure.
int value_copy(Value *dest, const Value *src);

// Promotes two values in-place so both share the same common supremum type
// in the hierarchy: VAL_INT -> VAL_DEC -> VAL_COMPLEX.
// Returns 1 on success, 0 on promotion error.
int value_promote(Value *a, Value *b);

// Formats any Value into a newly allocated, null-terminated display string.
// Caller must free the returned string with free().
char *value_to_string(const Value *v);

// ============================================================================
// Value Conversion Helpers
// ============================================================================

// Converts a Value (VAL_INT or VAL_DEC) into a BigDecimal representation.
// Returns 1 on success, 0 if incompatible (e.g., VAL_COMPLEX).
int value_to_decimal(BigDecimal *out, const Value *v);

// Converts a Value (VAL_INT, VAL_DEC, or VAL_COMPLEX) into a Complex number.
// Real values have their imaginary component set to 0.
// Returns 1 on success, 0 on failure.
int value_to_complex(Complex *out, const Value *v);

// ============================================================================
// AST Evaluation Operations
// ============================================================================

// Recursively evaluates an AST expression node n within context ctx,
// storing the computed result in out.
// Returns 1 on success, 0 on evaluation failure (error logged in ctx->error).
int eval_node(const ExprNode *n, EvalContext *ctx, Value *out);

// Evaluates a binary operator between left and right operand Values,
// handling automatic type promotion, zero-division guards, and integer
// promotion upon remainder detection.
// Returns 1 on success, 0 on failure.
int eval_binary(OpCode op, const Value *left, const Value *right, EvalContext *ctx, size_t pos, Value *out);

// Evaluates a unary operator (+, -, !) on a single operand Value.
// Returns 1 on success, 0 on failure.
int eval_unary(OpCode op, const Value *operand, EvalContext *ctx, size_t pos, Value *out);

// Evaluates a function call by function catalog ID with evaluated argument Values.
// Handles trigonometric angle conversions, domain validation, and dispatch.
// Returns 1 on success, 0 on failure.
int eval_call(int fn_id, const Value *args, size_t argc, EvalContext *ctx, size_t pos, Value *out);

#ifdef __cplusplus
}
#endif

#endif // EVAL_H


