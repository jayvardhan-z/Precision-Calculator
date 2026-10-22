// ============================================================================
// File: Worklet3/eval.c
// Description: Abstract Syntax Tree (AST) evaluation engine, symbol table,
//              type promotion system, and expression execution runtime.
//
// Overview:
// This file implements the runtime evaluator that traverses the AST generated
// by the recursive-descent parser and computes exact or high-precision results.
// Key responsibilities include:
// 1. Value Container Lifecycle: Initializing, copying, converting, and freeing
//    tagged unions supporting BigInteger, BigDecimal, and Complex types.
// 2. Type Hierarchy & Promotion: Automatically widening operands to their least
//    common supertype (VAL_INT -> VAL_DEC -> VAL_COMPLEX).
// 3. Integer to Decimal Promotion on Division: Detecting fractional division
//    remainders or negative integer exponents and seamlessly promoting to
//    arbitrary-precision BigDecimal.
// 4. Symbol Management: Dynamic symbol table lookup and mutation for user-defined
//    variables and preloaded constants (pi, e, i, ans).
// 5. AST Traversal: Depth-guarded recursive post-order tree walker evaluating
//    nodes bottom-up with exact character-position error reporting.
// ============================================================================

#define _POSIX_C_SOURCE 200809L

#include "eval.h"
#include "parser.h"
#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Value Lifecycle & Conversions
// ============================================================================

// Initializes a Value container to a blank, valid state for the specified type
void value_init(Value *v, ValueType type) {
    // Guard against NULL pointer dereferences
    if (!v) return;

    // Set tagged union discriminator
    v->type = type;

    // Initialize underlying type-specific structures
    switch (type) {
        case VAL_INT:
            // Initialize zero-value BigInteger
            bigint_init(&v->as.i);
            break;
        case VAL_DEC:
            // Initialize zero-value BigDecimal (scale = 0)
            decimal_init(&v->as.d);
            break;
        case VAL_COMPLEX:
            // Initialize complex number (0 + 0i)
            complex_init(&v->as.c);
            break;
    }
}

// Releases all dynamically allocated memory owned by the Value container
void value_free(Value *v) {
    // Guard against NULL pointer dereferences
    if (!v) return;

    // Dispatch appropriate cleanup based on active tagged union variant
    switch (v->type) {
        case VAL_INT:
            // Free limb storage of BigInteger
            bigint_free(&v->as.i);
            break;
        case VAL_DEC:
            // Free significand limb storage of BigDecimal
            decimal_free(&v->as.d);
            break;
        case VAL_COMPLEX:
            // Free both real and imaginary BigDecimal components
            complex_free(&v->as.c);
            break;
    }
}

// Creates an independent deep copy of a Value container
int value_copy(Value *dest, const Value *src) {
    // Validate destination and source pointers
    if (!dest || !src) return 0;

    // Clean up any existing resources in destination before overwriting
    value_free(dest);

    // Set destination variant tag to match source
    dest->type = src->type;

    // Deep copy payload depending on active variant
    switch (src->type) {
        case VAL_INT:
            bigint_init(&dest->as.i);
            return bigint_copy(&dest->as.i, &src->as.i);

        case VAL_DEC:
            decimal_init(&dest->as.d);
            return decimal_copy(&dest->as.d, &src->as.d);

        case VAL_COMPLEX:
            complex_init(&dest->as.c);
            return complex_copy(&dest->as.c, &src->as.c);
    }

    return 0;
}

// Converts any real Value (VAL_INT or VAL_DEC) into a BigDecimal
int value_to_decimal(BigDecimal *out, const Value *v) {
    if (!out || !v) return 0;

    // Clear target decimal and reinitialize
    decimal_free(out);
    decimal_init(out);

    // Perform conversion according to input type
    if (v->type == VAL_INT) {
        // Integer conversion: Copy integer limbs into significand with scale = 0
        if (!bigint_copy(&out->significand, &v->as.i)) return 0;
        out->scale = 0;
        return 1;
    } else if (v->type == VAL_DEC) {
        // Decimal conversion: Deep copy directly
        return decimal_copy(out, &v->as.d);
    }

    // Complex values cannot be implicitly converted to purely real BigDecimal
    return 0;
}

// Converts any Value (VAL_INT, VAL_DEC, or VAL_COMPLEX) into a Complex number
int value_to_complex(Complex *out, const Value *v) {
    if (!out || !v) return 0;

    if (v->type == VAL_INT) {
        // Convert integer into real part with scale 0, and imag part to 0
        BigDecimal r, i_zero;
        decimal_init(&r);
        decimal_init(&i_zero);

        bigint_copy(&r.significand, &v->as.i);
        r.scale = 0;

        complex_set(out, &r, &i_zero);

        decimal_free(&r);
        decimal_free(&i_zero);
        return 1;
    } else if (v->type == VAL_DEC) {
        // Real part takes decimal value, imag part is initialized to 0
        BigDecimal i_zero;
        decimal_init(&i_zero);

        complex_set(out, &v->as.d, &i_zero);

        decimal_free(&i_zero);
        return 1;
    } else if (v->type == VAL_COMPLEX) {
        // Direct deep copy for existing complex values
        return complex_copy(out, &v->as.c);
    }

    return 0;
}

// Lifts operand to a higher representation in place:
// VAL_INT -> VAL_DEC -> VAL_COMPLEX
static int lift_value_to(Value *v, ValueType target) {
    // If operand already matches target type, no conversion needed
    if (v->type == target) return 1;

    // Promotion 1: BigInteger -> BigDecimal
    if (v->type == VAL_INT && target == VAL_DEC) {
        BigDecimal dec;
        decimal_init(&dec);

        // Transfer integer limbs directly into decimal significand with scale = 0
        bigint_copy(&dec.significand, &v->as.i);
        dec.scale = 0;

        // Clean up old integer container and update tag
        bigint_free(&v->as.i);
        v->type = VAL_DEC;
        v->as.d = dec;
        return 1;
    }

    // Promotion 2: BigInteger -> Complex
    if (v->type == VAL_INT && target == VAL_COMPLEX) {
        Complex comp;
        complex_init(&comp);

        // Set real part significand from integer, imag remains 0
        bigint_copy(&comp.real.significand, &v->as.i);
        comp.real.scale = 0;

        // Clean up old integer container and update tag
        bigint_free(&v->as.i);
        v->type = VAL_COMPLEX;
        v->as.c = comp;
        return 1;
    }

    // Promotion 3: BigDecimal -> Complex
    if (v->type == VAL_DEC && target == VAL_COMPLEX) {
        Complex comp;
        complex_init(&comp);

        // Copy decimal value into real component, imag remains 0
        decimal_copy(&comp.real, &v->as.d);

        // Clean up old decimal container and update tag
        decimal_free(&v->as.d);
        v->type = VAL_COMPLEX;
        v->as.c = comp;
        return 1;
    }

    // Demotion or unsupported conversion requested
    return 0;
}

// Promotes both operands in-place to their common least upper bound type
int value_promote(Value *a, Value *b) {
    if (!a || !b) return 0;

    // Determine target type as the maximum enum value in the type hierarchy:
    // VAL_INT (0) < VAL_DEC (1) < VAL_COMPLEX (2)
    ValueType target = a->type;
    if (b->type > target) target = b->type;

    // Lift operand a to target type
    if (!lift_value_to(a, target)) return 0;

    // Lift operand b to target type
    if (!lift_value_to(b, target)) return 0;

    return 1;
}

// Formats any Value into a newly allocated null-terminated display string
char *value_to_string(const Value *v) {
    if (!v) return NULL;

    // Dispatch string conversion to underlying type formatter
    switch (v->type) {
        case VAL_INT:
            return bigint_to_string(&v->as.i);
        case VAL_DEC:
            return decimal_to_string(&v->as.d);
        case VAL_COMPLEX:
            return complex_to_string(&v->as.c);
    }

    return NULL;
}

// ============================================================================
// Symbol Table Implementation
// ============================================================================

// Initializes an empty SymbolTable
void symtab_init(SymbolTable *st) {
    if (!st) return;
    st->symbols = NULL;
    st->count = 0;
    st->capacity = 0;
}

// Frees all symbols, their identifier strings, and stored values
void symtab_free(SymbolTable *st) {
    if (!st) return;

    // Free each allocated symbol's name and value payload
    for (size_t i = 0; i < st->count; i++) {
        free(st->symbols[i].name);
        value_free(&st->symbols[i].value);
    }

    // Free contiguous symbol array
    free(st->symbols);
    st->symbols = NULL;
    st->count = 0;
    st->capacity = 0;
}

// Sets or updates a variable binding in the symbol table
int symtab_set(SymbolTable *st, const char *name, const Value *v) {
    if (!st || !name || !v) return 0;

    // Check if identifier is already registered; if so, overwrite existing value
    for (size_t i = 0; i < st->count; i++) {
        if (strcmp(st->symbols[i].name, name) == 0) {
            value_free(&st->symbols[i].value);
            return value_copy(&st->symbols[i].value, v);
        }
    }

    // Expand symbol table buffer capacity geometrically when full
    if (st->count >= st->capacity) {
        size_t new_cap = (st->capacity == 0 ? 8 : st->capacity * 2);
        Symbol *new_syms = (Symbol *)realloc(st->symbols, new_cap * sizeof(Symbol));
        if (!new_syms) return 0;
        st->symbols = new_syms;
        st->capacity = new_cap;
    }

    // Duplicate identifier name string onto heap
    st->symbols[st->count].name = strdup(name);
    if (!st->symbols[st->count].name) return 0;

    // Initialize and copy value into newly assigned symbol slot
    value_init(&st->symbols[st->count].value, v->type);
    if (!value_copy(&st->symbols[st->count].value, v)) {
        free(st->symbols[st->count].name);
        return 0;
    }

    st->count++;
    return 1;
}

// Retrieves a value bound to an identifier name
int symtab_get(const SymbolTable *st, const char *name, Value *out) {
    if (!st || !name || !out) return 0;

    // Linear search through symbol table
    for (size_t i = 0; i < st->count; i++) {
        if (strcmp(st->symbols[i].name, name) == 0) {
            // Return deep copy of found symbol value
            return value_copy(out, &st->symbols[i].value);
        }
    }

    // Identifier not found
    return 0;
}

// Pre-populates the symbol table with standard constants: pi, e, i, ans
int symtab_preload_constants(SymbolTable *st, int precision) {
    if (!st) return 0;

    BigDecimal pi_dec, e_dec;
    decimal_init(&pi_dec);
    decimal_init(&e_dec);

    // Compute mathematical constants with 8 extra guard digits
    math_const_pi(&pi_dec, precision + 8);
    math_const_e(&e_dec, precision + 8);

    Value v_pi, v_e, v_ans, v_i;
    v_pi.type = VAL_DEC; v_pi.as.d = pi_dec;
    v_e.type = VAL_DEC;  v_e.as.d = e_dec;

    // Initialize 'ans' to integer 0
    value_init(&v_ans, VAL_INT);
    bigint_clear(&v_ans.as.i);

    // Initialize imaginary unit 'i' = 0 + 1i
    value_init(&v_i, VAL_COMPLEX);
    BigDecimal i_zero, i_one;
    decimal_init(&i_zero);
    decimal_init(&i_one);
    decimal_from_string(&i_zero, "0");
    decimal_from_string(&i_one, "1");
    complex_set(&v_i.as.c, &i_zero, &i_one);
    decimal_free(&i_zero);
    decimal_free(&i_one);

    // Register all constants into symbol table
    int ok = symtab_set(st, "pi", &v_pi) &&
             symtab_set(st, "e", &v_e) &&
             symtab_set(st, "i", &v_i) &&
             symtab_set(st, "ans", &v_ans);

    // Free local Value wrappers (symtab_set made deep copies)
    value_free(&v_pi);
    value_free(&v_e);
    value_free(&v_i);
    value_free(&v_ans);
    return ok;
}

// ============================================================================
// Evaluation Context & Error Handling
// ============================================================================

// Initializes runtime evaluation context with default parameters
void eval_context_init(EvalContext *ctx, SymbolTable *st) {
    if (!ctx) return;
    ctx->precision = 20;            // Default 20 fractional digits
    ctx->angle_mode = ANGLE_RADIAN; // Default radian mode
    ctx->rounding = ROUND_HALF_UP;  // Default round half-up tie breaking
    ctx->vars = st;                 // Attach symbol table pointer
    ctx->depth = 0;                 // Reset recursion depth counter
    ctx->error.code = ERR_NONE;     // Clear error latch
    ctx->error.pos = 0;
    ctx->error.message[0] = '\0';
}

// Latches an evaluation error into the context (only records the FIRST error)
static void set_error(EvalContext *ctx, EvalErrorCode code, size_t pos, const char *msg) {
    // If context is NULL or an earlier error has already been recorded, preserve it
    if (!ctx || ctx->error.code != ERR_NONE) return;

    ctx->error.code = code;
    ctx->error.pos = pos;
    snprintf(ctx->error.message, sizeof(ctx->error.message), "%s", msg);
}

// ============================================================================
// Factorial Helper on BigInteger
// ============================================================================

// Computes factorial n! = n * (n-1) * ... * 1 for arbitrary precision integer n
static int bigint_factorial(BigInteger *res, const BigInteger *n) {
    // Negative numbers have undefined factorial
    if (n->sign < 0) return 0;

    // Base cases: 0! = 1
    if (bigint_is_zero(n)) {
        bigint_set_u64(res, 1);
        return 1;
    }

    // Convert BigInteger to uint64 for practical iteration
    char *s = bigint_to_string(n);
    if (!s) return 0;
    unsigned long long n_val = strtoull(s, NULL, 10);
    free(s);

    // Safety guard: Limit factorial operand to prevent excessive computation time
    if (n_val > 10000) return 0;

    // Iterative multiplication: res = 1 * 2 * 3 * ... * n_val
    bigint_set_u64(res, 1);
    for (unsigned long long i = 2; i <= n_val; i++) {
        // Multiply accumulating BigInteger by scalar loop index i
        if (!bigint_mul_small(res, res, (uint32_t)i)) return 0;
    }

    return 1;
}

// ============================================================================
// Binary Operations Evaluator
// ============================================================================

// Evaluates binary operator between two Values, handling promotion and execution
int eval_binary(OpCode op, const Value *left, const Value *right, EvalContext *ctx, size_t pos, Value *out) {
    if (!left || !right || !ctx || !out) return 0;
    value_free(out);

    // Make local copies of left and right operands for promotion
    Value l, r;
    value_init(&l, left->type);  value_copy(&l, left);
    value_init(&r, right->type); value_copy(&r, right);

    // Promote both operands to common supertype (INT -> DEC -> COMPLEX)
    if (!value_promote(&l, &r)) {
        value_free(&l);
        value_free(&r);
        set_error(ctx, ERR_TYPE_MISMATCH, pos, "Operand promotion failed");
        return 0;
    }

    int success = 0;

    // ------------------------------------------------------------------------
    // Branch 1: Both Operands are BigIntegers
    // ------------------------------------------------------------------------
    if (l.type == VAL_INT) {
        value_init(out, VAL_INT);

        switch (op) {
            case OP_ADD:
                // Integer addition: out = l + r
                success = bigint_add(&out->as.i, &l.as.i, &r.as.i);
                break;

            case OP_SUB:
                // Integer subtraction: out = l - r
                success = bigint_sub(&out->as.i, &l.as.i, &r.as.i);
                break;

            case OP_MUL:
                // Integer multiplication: out = l * r
                success = bigint_mul(&out->as.i, &l.as.i, &r.as.i);
                break;

            case OP_DIV:
                // Division by zero check
                if (bigint_is_zero(&r.as.i)) {
                    set_error(ctx, ERR_DIV_BY_ZERO, pos, "Division by zero");
                    success = 0;
                } else {
                    DivisionResult dres;
                    division_result_init(&dres);

                    if (bigint_divide(&dres, &l.as.i, &r.as.i)) {
                        // If division has a non-zero remainder, automatically promote
                        // the result to BigDecimal to retain full decimal precision!
                        if (!bigint_is_zero(&dres.remainder)) {
                            value_free(out);
                            value_init(out, VAL_DEC);

                            BigDecimal num, den;
                            decimal_init(&num);
                            decimal_init(&den);

                            // Copy integer operands to decimal representation
                            bigint_copy(&num.significand, &l.as.i);
                            bigint_copy(&den.significand, &r.as.i);

                            // Perform decimal division to target context precision
                            success = decimal_div(&out->as.d, &num, &den, ctx->precision);

                            decimal_free(&num);
                            decimal_free(&den);
                        } else {
                            // Remainder is zero: Integer division is exact
                            bigint_copy(&out->as.i, &dres.quotient);
                            success = 1;
                        }
                    }
                    division_result_free(&dres);
                }
                break;

            case OP_MOD:
                // Modulo by zero check
                if (bigint_is_zero(&r.as.i)) {
                    set_error(ctx, ERR_DIV_BY_ZERO, pos, "Modulo by zero");
                    success = 0;
                } else {
                    // Integer remainder: out = l mod r
                    success = bigint_mod(&out->as.i, &l.as.i, &r.as.i);
                }
                break;

            case OP_POW: {
                // Convert exponent to string to inspect magnitude and sign
                char *exp_str = bigint_to_string(&r.as.i);
                if (!exp_str) {
                    success = 0;
                    break;
                }

                if (r.as.i.sign < 0) {
                    // Negative exponent (e.g. 2^-3 = 1/8):
                    // Automatically promote result to BigDecimal representation
                    value_free(out);
                    value_init(out, VAL_DEC);

                    BigDecimal b_dec, res_dec, one;
                    decimal_init(&b_dec);
                    decimal_init(&res_dec);
                    decimal_init(&one);

                    bigint_copy(&b_dec.significand, &l.as.i);
                    decimal_from_string(&one, "1");

                    // Compute base^|exponent| as integer
                    uint64_t u_exp = strtoull(exp_str + 1, NULL, 10);
                    BigInteger p_int;
                    bigint_init(&p_int);
                    bigint_pow(&p_int, &l.as.i, u_exp);
                    bigint_copy(&res_dec.significand, &p_int);

                    // Compute reciprocal: out = 1 / (base^|exponent|)
                    success = decimal_div(&out->as.d, &one, &res_dec, ctx->precision);

                    bigint_free(&p_int);
                    decimal_free(&b_dec);
                    decimal_free(&res_dec);
                    decimal_free(&one);
                } else {
                    // Non-negative exponent: Exact integer power via repeated squaring
                    uint64_t u_exp = strtoull(exp_str, NULL, 10);
                    success = bigint_pow(&out->as.i, &l.as.i, u_exp);
                }
                free(exp_str);
                break;
            }

            default:
                set_error(ctx, ERR_INTERNAL, pos, "Unsupported operator on integer");
                break;
        }

    // ------------------------------------------------------------------------
    // Branch 2: Both Operands are BigDecimals
    // ------------------------------------------------------------------------
    } else if (l.type == VAL_DEC) {
        value_init(out, VAL_DEC);

        switch (op) {
            case OP_ADD:
                // Decimal addition with scale alignment: out = l + r
                success = decimal_add(&out->as.d, &l.as.d, &r.as.d);
                break;

            case OP_SUB:
                // Decimal subtraction with scale alignment: out = l - r
                success = decimal_sub(&out->as.d, &l.as.d, &r.as.d);
                break;

            case OP_MUL:
                // Decimal multiplication (scale = scale_l + scale_r): out = l * r
                success = decimal_mul(&out->as.d, &l.as.d, &r.as.d);
                break;

            case OP_DIV:
                // Decimal division by zero check
                if (decimal_is_zero(&r.as.d)) {
                    set_error(ctx, ERR_DIV_BY_ZERO, pos, "Division by zero");
                    success = 0;
                } else {
                    // Decimal division with quotient shifted to target precision
                    success = decimal_div(&out->as.d, &l.as.d, &r.as.d, ctx->precision);
                }
                break;

            case OP_POW: {
                // General decimal power via identity: l^r = exp(r * ln(l))
                if (l.as.d.significand.sign <= 0) {
                    set_error(ctx, ERR_DOMAIN, pos, "Negative base for decimal power");
                    success = 0;
                } else {
                    BigDecimal ln_l, prod;
                    decimal_init(&ln_l);
                    decimal_init(&prod);

                    // Compute ln(l) with guard precision, multiply by r, then take exp()
                    if (fn_ln(&ln_l, &l.as.d, ctx->precision + 8) &&
                        decimal_mul(&prod, &r.as.d, &ln_l)) {
                        success = fn_exp(&out->as.d, &prod, ctx->precision);
                    }

                    decimal_free(&ln_l);
                    decimal_free(&prod);
                }
                break;
            }

            default:
                set_error(ctx, ERR_INTERNAL, pos, "Unsupported operator on decimal");
                break;
        }

    // ------------------------------------------------------------------------
    // Branch 3: Both Operands are Complex Numbers
    // ------------------------------------------------------------------------
    } else if (l.type == VAL_COMPLEX) {
        value_init(out, VAL_COMPLEX);

        switch (op) {
            case OP_ADD:
                // Complex addition: (a + bi) + (c + di) = (a+c) + (b+d)i
                success = complex_add(&out->as.c, &l.as.c, &r.as.c);
                break;

            case OP_SUB:
                // Complex subtraction: (a + bi) - (c + di) = (a-c) + (b-d)i
                success = complex_sub(&out->as.c, &l.as.c, &r.as.c);
                break;

            case OP_MUL:
                // Complex multiplication: (ac - bd) + (ad + bc)i
                success = complex_mul(&out->as.c, &l.as.c, &r.as.c);
                break;

            case OP_DIV:
                // Complex division by zero check: c^2 + d^2 == 0
                if (complex_is_zero(&r.as.c)) {
                    set_error(ctx, ERR_DIV_BY_ZERO, pos, "Division by complex zero");
                    success = 0;
                } else {
                    // Complex conjugate division scaled to target precision
                    success = complex_div(&out->as.c, &l.as.c, &r.as.c, ctx->precision);
                }
                break;

            default:
                set_error(ctx, ERR_INTERNAL, pos, "Unsupported operator on complex");
                break;
        }
    }

    // Free promoted temporary values
    value_free(&l);
    value_free(&r);
    return success;
}

// ============================================================================
// Unary Operations Evaluator
// ============================================================================

// Evaluates unary operations (+x, -x, x!) on operand Value
int eval_unary(OpCode op, const Value *operand, EvalContext *ctx, size_t pos, Value *out) {
    if (!operand || !ctx || !out) return 0;
    value_free(out);

    int success = 0;

    // Unary plus: Identity operation (+x = x)
    if (op == OP_POS) {
        return value_copy(out, operand);

    // Unary negation: Invert sign of operand (-x)
    } else if (op == OP_NEG) {
        if (!value_copy(out, operand)) return 0;

        // Invert sign of active numeric payload
        if (out->type == VAL_INT) {
            out->as.i.sign = -out->as.i.sign;
        } else if (out->type == VAL_DEC) {
            out->as.d.significand.sign = -out->as.d.significand.sign;
        } else if (out->type == VAL_COMPLEX) {
            // For complex numbers, negate both real and imaginary components
            out->as.c.real.significand.sign = -out->as.c.real.significand.sign;
            out->as.c.imag.significand.sign = -out->as.c.imag.significand.sign;
        }
        return 1;

    // Postfix factorial: x!
    } else if (op == OP_FACT) {
        // Factorial is strictly defined on non-negative integers
        if (operand->type != VAL_INT) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "Factorial requires non-negative integer");
            return 0;
        }

        value_init(out, VAL_INT);
        success = bigint_factorial(&out->as.i, &operand->as.i);
        if (!success) {
            set_error(ctx, ERR_DOMAIN, pos, "Factorial input negative or too large");
        }
        return success;
    }

    set_error(ctx, ERR_INTERNAL, pos, "Unknown unary operator");
    return 0;
}

// ============================================================================
// Function Call Evaluator
// ============================================================================

// Dispatches built-in function calls by ID with evaluated arguments
int eval_call(int fn_id, const Value *args, size_t argc, EvalContext *ctx, size_t pos, Value *out) {
    (void)argc; // Arity is already validated by the parser
    if (!args || !ctx || !out) return 0;
    value_free(out);

    // Validate function catalog index
    if (fn_id < 0 || (size_t)fn_id >= FUNCTION_COUNT) {
        set_error(ctx, ERR_UNDEFINED_FUNC, pos, "Invalid function ID");
        return 0;
    }

    const char *name = FUNCTIONS[fn_id].name;
    int success = 0;

    // ------------------------------------------------------------------------
    // Trigonometric Function: sin(x)
    // ------------------------------------------------------------------------
    if (strcmp(name, "sin") == 0) {
        BigDecimal dec, rad;
        decimal_init(&dec);
        decimal_init(&rad);

        if (!value_to_decimal(&dec, &args[0])) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "sin requires numeric argument");
        } else {
            // In degree mode, convert degrees to radians: rad = dec * pi / 180
            if (ctx->angle_mode == ANGLE_DEGREE) {
                BigDecimal pi, one_eighty;
                decimal_init(&pi);
                decimal_init(&one_eighty);

                math_const_pi(&pi, ctx->precision + 8);
                decimal_from_string(&one_eighty, "180");
                decimal_mul(&rad, &dec, &pi);
                decimal_div(&rad, &rad, &one_eighty, ctx->precision + 8);

                decimal_free(&pi);
                decimal_free(&one_eighty);
            } else {
                decimal_copy(&rad, &dec);
            }

            value_init(out, VAL_DEC);
            success = fn_sin(&out->as.d, &rad, ctx->precision);
        }

        decimal_free(&dec);
        decimal_free(&rad);

    // ------------------------------------------------------------------------
    // Trigonometric Function: cos(x)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "cos") == 0) {
        BigDecimal dec, rad;
        decimal_init(&dec);
        decimal_init(&rad);

        if (!value_to_decimal(&dec, &args[0])) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "cos requires numeric argument");
        } else {
            // In degree mode, convert degrees to radians: rad = dec * pi / 180
            if (ctx->angle_mode == ANGLE_DEGREE) {
                BigDecimal pi, one_eighty;
                decimal_init(&pi);
                decimal_init(&one_eighty);

                math_const_pi(&pi, ctx->precision + 8);
                decimal_from_string(&one_eighty, "180");
                decimal_mul(&rad, &dec, &pi);
                decimal_div(&rad, &rad, &one_eighty, ctx->precision + 8);

                decimal_free(&pi);
                decimal_free(&one_eighty);
            } else {
                decimal_copy(&rad, &dec);
            }

            value_init(out, VAL_DEC);
            success = fn_cos(&out->as.d, &rad, ctx->precision);
        }

        decimal_free(&dec);
        decimal_free(&rad);

    // ------------------------------------------------------------------------
    // Exponential Function: exp(x)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "exp") == 0) {
        BigDecimal dec;
        decimal_init(&dec);

        if (!value_to_decimal(&dec, &args[0])) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "exp requires numeric argument");
        } else {
            value_init(out, VAL_DEC);
            success = fn_exp(&out->as.d, &dec, ctx->precision);
        }

        decimal_free(&dec);

    // ------------------------------------------------------------------------
    // Natural Logarithm: ln(x)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "ln") == 0) {
        BigDecimal dec;
        decimal_init(&dec);

        if (!value_to_decimal(&dec, &args[0])) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "ln requires numeric argument");
        } else {
            value_init(out, VAL_DEC);
            success = fn_ln(&out->as.d, &dec, ctx->precision);
            if (!success) {
                set_error(ctx, ERR_DOMAIN, pos, "Domain error: ln argument <= 0");
            }
        }

        decimal_free(&dec);

    // ------------------------------------------------------------------------
    // Square Root: sqrt(x)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "sqrt") == 0) {
        BigDecimal dec;
        decimal_init(&dec);

        if (!value_to_decimal(&dec, &args[0])) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "sqrt requires numeric argument");
        } else {
            value_init(out, VAL_DEC);
            success = fn_sqrt(&out->as.d, &dec, ctx->precision);
            if (!success) {
                set_error(ctx, ERR_DOMAIN, pos, "Domain error: negative sqrt argument");
            }
        }

        decimal_free(&dec);

    // ------------------------------------------------------------------------
    // Absolute Value / Modulus: abs(x)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "abs") == 0) {
        if (args[0].type == VAL_INT) {
            // Integer absolute value: clear negative sign
            value_init(out, VAL_INT);
            bigint_copy(&out->as.i, &args[0].as.i);
            out->as.i.sign = (bigint_is_zero(&out->as.i) ? 0 : 1);
            success = 1;
        } else if (args[0].type == VAL_DEC) {
            // Decimal absolute value: clear negative sign
            value_init(out, VAL_DEC);
            decimal_copy(&out->as.d, &args[0].as.d);
            out->as.d.significand.sign = (decimal_is_zero(&out->as.d) ? 0 : 1);
            success = 1;
        } else if (args[0].type == VAL_COMPLEX) {
            // Complex modulus: |a + bi| = sqrt(a^2 + b^2)
            BigDecimal r2, i2, sum_sq;
            decimal_init(&r2);
            decimal_init(&i2);
            decimal_init(&sum_sq);

            // Compute a^2 and b^2
            decimal_mul(&r2, &args[0].as.c.real, &args[0].as.c.real);
            decimal_mul(&i2, &args[0].as.c.imag, &args[0].as.c.imag);

            // Sum of squares: a^2 + b^2
            decimal_add(&sum_sq, &r2, &i2);

            // Take square root of sum of squares
            value_init(out, VAL_DEC);
            success = fn_sqrt(&out->as.d, &sum_sq, ctx->precision);

            decimal_free(&r2);
            decimal_free(&i2);
            decimal_free(&sum_sq);
        }

    // ------------------------------------------------------------------------
    // Greatest Common Divisor: gcd(a, b)
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "gcd") == 0) {
        if (args[0].type != VAL_INT || args[1].type != VAL_INT) {
            set_error(ctx, ERR_TYPE_MISMATCH, pos, "gcd requires integer arguments");
        } else {
            value_init(out, VAL_INT);
            success = bigint_gcd(&out->as.i, &args[0].as.i, &args[1].as.i);
        }

    // ------------------------------------------------------------------------
    // Function Aliases for Operators
    // ------------------------------------------------------------------------
    } else if (strcmp(name, "pow") == 0) {
        // Alias pow(a, b) -> a ^ b
        return eval_binary(OP_POW, &args[0], &args[1], ctx, pos, out);
    } else if (strcmp(name, "mod") == 0) {
        // Alias mod(a, b) -> a % b
        return eval_binary(OP_MOD, &args[0], &args[1], ctx, pos, out);
    } else if (strcmp(name, "fact") == 0) {
        // Alias fact(n) -> n!
        return eval_unary(OP_FACT, &args[0], ctx, pos, out);
    } else {
        set_error(ctx, ERR_UNDEFINED_FUNC, pos, "Unrecognized function");
    }

    return success;
}

// ============================================================================
// Recursive Post-Order AST Tree Walker
// ============================================================================

// Evaluates an AST expression node recursively, returning result Value
int eval_node(const ExprNode *n, EvalContext *ctx, Value *out) {
    if (!n || !ctx || !out) return 0;

    // Stack overflow guard: abort if recursion depth exceeds safety threshold
    if (ctx->depth > MAX_RECURSION_DEPTH) {
        set_error(ctx, ERR_RECURSION_LIMIT, n->pos, "Recursion depth limit exceeded");
        return 0;
    }

    // Track recursion descent depth
    ctx->depth++;
    int success = 0;
    value_free(out);

    // Evaluate based on AST node variant
    switch (n->type) {
        // --------------------------------------------------------------------
        // Leaf Node: Numeric Literal
        // --------------------------------------------------------------------
        case NODE_NUMBER:
            // Simply copy literal value from node payload to output
            success = value_copy(out, &n->as.number);
            break;

        // --------------------------------------------------------------------
        // Leaf Node: Variable / Constant Lookup
        // --------------------------------------------------------------------
        case NODE_VAR:
            // Look up identifier name in symbol table
            if (!symtab_get(ctx->vars, n->as.var_name, out)) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Undefined variable '%s'", n->as.var_name);
                set_error(ctx, ERR_UNDEFINED_VAR, n->pos, msg);
                success = 0;
            } else {
                success = 1;
            }
            break;

        // --------------------------------------------------------------------
        // Internal Node: Binary Operator (Post-Order: Left, Right, Operator)
        // --------------------------------------------------------------------
        case NODE_BINARY: {
            Value l, r;
            value_init(&l, VAL_INT);
            value_init(&r, VAL_INT);

            // Recursively evaluate left child, then right child
            if (eval_node(n->as.bin.left, ctx, &l) && eval_node(n->as.bin.right, ctx, &r)) {
                // Apply binary operator to evaluated children
                success = eval_binary(n->as.bin.op, &l, &r, ctx, n->pos, out);
            }

            // Clean up intermediate subtree values
            value_free(&l);
            value_free(&r);
            break;
        }

        // --------------------------------------------------------------------
        // Internal Node: Unary Operator (Post-Order: Child, Operator)
        // --------------------------------------------------------------------
        case NODE_UNARY: {
            Value operand;
            value_init(&operand, VAL_INT);

            // Recursively evaluate single child operand
            if (eval_node(n->as.un.operand, ctx, &operand)) {
                // Apply unary operator
                success = eval_unary(n->as.un.op, &operand, ctx, n->pos, out);
            }

            // Clean up intermediate operand value
            value_free(&operand);
            break;
        }

        // --------------------------------------------------------------------
        // Internal Node: Function Call (Post-Order: Arguments, Call)
        // --------------------------------------------------------------------
        case NODE_CALL: {
            // Allocate temporary array to hold evaluated argument Values
            Value *eval_args = (Value *)calloc(n->as.call.argc, sizeof(Value));
            if (!eval_args && n->as.call.argc > 0) {
                set_error(ctx, ERR_INTERNAL, n->pos, "Memory allocation failure");
                ctx->depth--;
                return 0;
            }

            int all_args_ok = 1;

            // Recursively evaluate each argument expression node
            for (size_t i = 0; i < n->as.call.argc; i++) {
                value_init(&eval_args[i], VAL_INT);
                if (!eval_node(n->as.call.args[i], ctx, &eval_args[i])) {
                    all_args_ok = 0;
                    break;
                }
            }

            // If all argument subtrees evaluated successfully, dispatch function
            if (all_args_ok) {
                success = eval_call(n->as.call.fn_id, eval_args, n->as.call.argc, ctx, n->pos, out);
            }

            // Clean up and free evaluated argument values
            for (size_t i = 0; i < n->as.call.argc; i++) {
                value_free(&eval_args[i]);
            }
            free(eval_args);
            break;
        }
    }

    // Step back up recursion depth counter
    ctx->depth--;
    return success;
}

