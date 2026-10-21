#ifndef COMPLEX_H
#define COMPLEX_H

#include "decimal.h"

// ============================================================================
// File: complex.h
// Purpose: Arbitrary-precision complex number engine (a + bi).
// Role:
//   Represents complex numbers whose real and imaginary components are
//   independent arbitrary-precision BigDecimal values.
//
// Mathematical Invariants:
//   z = real + imag * i, where i = sqrt(-1)
//   Addition:       (a + bi) + (c + di) = (a + c) + (b + d)i
//   Subtraction:    (a + bi) - (c + di) = (a - c) + (b - d)i
//   Multiplication: (a + bi) * (c + di) = (ac - bd) + (ad + bc)i
//   Division:       (a + bi) / (c + di) = ((ac + bd) + (bc - ad)i) / (c^2 + d^2)
// ============================================================================

// Complex number structure with arbitrary-precision decimal parts
typedef struct {
    BigDecimal real; // Real component (a in a + bi)
    BigDecimal imag; // Imaginary component (b in a + bi)
} Complex;

// ----------------------------------------------------------------------------
// Lifecycle & String I/O
// ----------------------------------------------------------------------------

// Initializes complex number with real = 0, imag = 0
void  complex_init(Complex *c);

// Frees heap resources allocated within real and imaginary decimal components
void  complex_free(Complex *c);

// Deep copies complex number from src to dest
int   complex_copy(Complex *dest, const Complex *src);

// Swaps two Complex structures in O(1) time
void  complex_swap(Complex *a, Complex *b);

// Assigns real and imaginary decimal values to complex structure
int   complex_set(Complex *c, const BigDecimal *real, const BigDecimal *imag);

// Sets complex components from raw decimal strings
int   complex_set_str(Complex *c, const char *real_str, const char *imag_str);

// Serializes complex number into string: handles purely real, purely imaginary ("bi"), and "a + bi" / "a - bi"
char *complex_to_string(const Complex *c);

// ----------------------------------------------------------------------------
// Complex Arithmetic
// ----------------------------------------------------------------------------

// Component-wise addition: (a + bi) + (c + di) = (a + c) + (b + d)i
int   complex_add(Complex *res, const Complex *a, const Complex *b);

// Component-wise subtraction: (a + bi) - (c + di) = (a - c) + (b - d)i
int   complex_sub(Complex *res, const Complex *a, const Complex *b);

// Complex multiplication: (ac - bd) + (ad + bc)i
int   complex_mul(Complex *res, const Complex *a, const Complex *b);

// Complex division via conjugate multiplication: ((ac + bd) + (bc - ad)i) / (c^2 + d^2)
int   complex_div(Complex *res, const Complex *a, const Complex *b, int precision);

// Checks if both real and imaginary components are zero
int   complex_is_zero(const Complex *c);

#endif /* COMPLEX_H */


