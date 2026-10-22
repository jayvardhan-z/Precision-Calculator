#ifndef FAST_MATH_H
#define FAST_MATH_H

#include "bigint.h"

// ============================================================================
// File: fast_math.h
// Purpose: Advanced arithmetic engine: Karatsuba multiplication, Knuth
//          Algorithm D long division, and number theory primitives.
// Role:
//   Implements sub-quadratic multiplication (O(N^1.585)) via Karatsuba divide-
//   and-conquer, multi-limb long division with quotient digit estimation and
//   add-back correction (Knuth Algorithm D), Euclidean GCD, and binary power.
// ============================================================================

// Crossover threshold in limbs between Schoolbook O(N^2) and Karatsuba O(N^1.585)
// Empirical tuning shows schoolbook is faster below 16 limbs (~144 decimal digits)
#define KARATSUBA_CROSSOVER 16

// ----------------------------------------------------------------------------
// Division Result Container
// ----------------------------------------------------------------------------

// Container holding the quotient and remainder resulting from integer division
typedef struct {
    BigInteger quotient;  // Integer quotient Q = floor(dividend / divisor)
    BigInteger remainder; // Integer remainder R = dividend - Q * divisor
} DivisionResult;

// Initializes both quotient and remainder to canonical zero
void division_result_init(DivisionResult *res);

// Frees the limbs allocated inside quotient and remainder
void division_result_free(DivisionResult *res);

// ----------------------------------------------------------------------------
// Fast Multiplication & Shift Utilities
// ----------------------------------------------------------------------------

// Shifts BigInteger left by 'shift' limbs (equivalent to multiplying by BASE^shift)
int  bigint_shift_limbs(BigInteger *res, const BigInteger *a, size_t shift);

// Slices a contiguous sub-range of 'len' limbs starting at 'start' into dest
int  bigint_slice(BigInteger *dest, const BigInteger *src, size_t start, size_t len);

// Karatsuba divide-and-conquer multiplication: res = a * b in O(N^1.585)
int  bigint_mul_karatsuba(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Adaptive multiplication: delegates to Schoolbook or Karatsuba depending on limb count
int  bigint_mul(BigInteger *res, const BigInteger *a, const BigInteger *b);

// ----------------------------------------------------------------------------
// Multi-Limb Long Division & Modulo (Knuth Algorithm D)
// ----------------------------------------------------------------------------

// Full multi-limb integer division: computes res->quotient and res->remainder
// Precondition: divisor != 0; returns 1 on success, 0 on division by zero or memory failure
int  bigint_divide(DivisionResult *res, const BigInteger *dividend, const BigInteger *divisor);

// Computes remainder res = a % b using bigint_divide
int  bigint_mod(BigInteger *res, const BigInteger *a, const BigInteger *b);

// ----------------------------------------------------------------------------
// Number Theory Primitives
// ----------------------------------------------------------------------------

// Euclidean algorithm for Greatest Common Divisor: res = gcd(|a|, |b|)
int  bigint_gcd(BigInteger *res, const BigInteger *a, const BigInteger *b);

// Exponentiation by repeated squaring (binary exponentiation): res = base^exp in O(log exp)
int  bigint_pow(BigInteger *res, const BigInteger *base, uint64_t exp);

// Modular exponentiation: res = (base^exp) % mod using binary exponentiation
int  bigint_mod_pow(BigInteger *res, const BigInteger *base, const BigInteger *exp, const BigInteger *mod);

#endif // FAST_MATH_H


