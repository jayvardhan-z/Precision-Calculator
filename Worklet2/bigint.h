#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// File: bigint.h
// Purpose: Core arbitrary-precision integer arithmetic engine (Base-10^9).
// Role:
//   Represents unbounded integers as dynamically-sized arrays of 32-bit limbs.
//   Each limb stores 9 decimal digits (Base 1,000,000,000).
//
// Architectural Invariants & Design Principles:
//   1. Base Selection (B = 10^9):
//      - Fits inside uint32_t (< 2^32 = 4,294,967,296).
//      - The product of two max limbs (10^9 - 1)^2 < 10^18 < 2^63 - 1, which fits
//        within a standard unsigned 64-bit uint64_t without 64-bit overflow.
//      - Direct 9-digit decimal alignment enables instantaneous decimal I/O without
//        expensive radix conversion divisions.
//   2. Endianness:
//      - Limbs are stored in little-endian order (least significant limb at digits[0]).
//   3. Canonical Zero Invariant:
//      - A normalized value of 0 is ALWAYS represented as:
//        size = 1, digits[0] = 0, sign = 0.
//   4. Sign Representation:
//      - sign is +1 for positive integers, -1 for negative integers, and 0 for zero.
// ============================================================================

// Base modulus for each limb (1 billion)
#define BASE 1000000000U
// Decimal digits packed inside a single limb
#define BASE_DIGITS 9

// Arbitrary-precision integer structure
typedef struct {
    int        sign;     // Sign indicator: +1 (positive), -1 (negative), 0 (zero)
    size_t     size;     // Number of active limbs currently in use (>= 1)
    size_t     capacity; // Number of limbs allocated in the digits buffer
    uint32_t  *digits;   // Dynamically allocated array of base-10^9 limbs (least significant first)
} BigInteger;

// ----------------------------------------------------------------------------
// Lifecycle, Normalization & Memory Management
// ----------------------------------------------------------------------------

// Initializes BigInteger to canonical zero (size = 1, capacity = 1, digits[0] = 0, sign = 0)
void bigint_init(BigInteger *a);

// Initializes BigInteger with a pre-allocated limb capacity
int  bigint_init_capacity(BigInteger *a, size_t cap);

// Ensures that the limb buffer can hold at least min_cap limbs, reallocating if needed
int  bigint_reserve(BigInteger *a, size_t min_cap);

// Resizes active limb count, allocating additional limbs and zero-initializing new space if expanded
int  bigint_resize(BigInteger *a, size_t new_size);

// Resets value to zero without freeing allocated capacity
void bigint_clear(BigInteger *a);

// Frees the heap-allocated digits buffer and resets all fields to 0
void bigint_free(BigInteger *a);

// Deep copies the value of src into dest (reallocating dest buffer if needed)
int  bigint_copy(BigInteger *dest, const BigInteger *src);

// Swaps the contents of two BigInteger structures in O(1) time
void bigint_swap(BigInteger *a, BigInteger *b);

// Strips redundant leading zero limbs and enforces the canonical zero invariant
void bigint_normalize(BigInteger *a);

// ----------------------------------------------------------------------------
// Value Inspection & Predicates
// ----------------------------------------------------------------------------

// Returns 1 if a equals 0, otherwise 0
int  bigint_is_zero(const BigInteger *a);

// Returns 1 if a equals +1, otherwise 0
int  bigint_is_one(const BigInteger *a);

// ----------------------------------------------------------------------------
// Comparisons
// ----------------------------------------------------------------------------

// Compares absolute values |a| and |b|: returns +1 if |a| > |b|, -1 if |a| < |b|, 0 if |a| == |b|
int  bigint_compare_abs(const BigInteger *a, const BigInteger *b);

// Signed comparison: returns +1 if a > b, -1 if a < b, 0 if a == b
int  bigint_compare(const BigInteger *a, const BigInteger *b);

// Relational predicate helpers
int  bigint_equal(const BigInteger *a, const BigInteger *b);
int  bigint_less(const BigInteger *a, const BigInteger *b);
int  bigint_greater(const BigInteger *a, const BigInteger *b);
int  bigint_less_equal(const BigInteger *a, const BigInteger *b);
int  bigint_greater_equal(const BigInteger *a, const BigInteger *b);

// ----------------------------------------------------------------------------
// Addition & Subtraction
// ----------------------------------------------------------------------------

// Unsigned magnitude addition: res = |a| + |b| with carry propagation
int  bigint_add_abs(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Unsigned magnitude subtraction: res = |a| - |b| with borrow propagation (assumes |a| >= |b|)
int  bigint_sub_abs(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Full signed addition: res = a + b (dispatches to add_abs or sub_abs depending on signs)
int  bigint_add(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Full signed subtraction: res = a - b (computed as a + (-b))
int  bigint_sub(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Scalar addition helper: adds a 64-bit signed integer to a BigInteger
int  bigint_add_small(BigInteger *res, const BigInteger *a, int64_t val);

// ----------------------------------------------------------------------------
// Multiplication
// ----------------------------------------------------------------------------

// Multiplies BigInteger by a single 32-bit unsigned limb (val < BASE): res = a * val
int  bigint_mul_small(BigInteger *res, const BigInteger *a, uint32_t val);

// Classical schoolbook column multiplication: res = a * b in O(N * M) time
int  bigint_mul_schoolbook(BigInteger *res, const BigInteger *a, const BigInteger *b);

// ----------------------------------------------------------------------------
// Numeric Setter Helpers
// ----------------------------------------------------------------------------

// Sets BigInteger value from an unsigned 64-bit scalar integer
int  bigint_set_u64(BigInteger *a, uint64_t val);

// Sets BigInteger value from a signed 64-bit scalar integer
int  bigint_set_i64(BigInteger *a, int64_t val);

#endif /* BIGINT_H */