// ============================================================================
// File: Worklet3/functions.h
// Description: Arbitrary-precision transcendental, scientific, and mathematical
//              function declarations operating on BigDecimal objects.
//
// Overview:
// This header declares functions for computing mathematical constants (pi, e, ln10)
// and transcendental/scientific functions (sqrt, exp, ln, sin, cos) to arbitrary
// user-specified decimal precisions.
//
// Key Algorithmic Principles:
// 1. Guard Precision: Internal calculations employ extra fractional digits
//    (typically precision + 8 to precision + 12) to shield results against
//    accumulated round-off error.
// 2. Range Reduction: Inputs to transcendental functions are shifted into optimal
//    convergence regions before series expansion (e.g., modulo 2*pi for trigonometric
//    functions, power-of-10 scaling for logarithms, integer/fractional split for exp).
// 3. Iterative Numerical Methods: Newton-Raphson iteration for square roots and
//    Maclaurin/Taylor series expansions with dynamic epsilon threshold convergence.
// ============================================================================

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// Arbitrary-precision fixed-point decimal definitions
#include "decimal.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Mathematical Constants (Accurate to >= 120 decimal digits)
// ============================================================================

// Computes or retrieves constant pi (3.1415926535...) rounded to target precision.
// Parameters:
//   out       - Target BigDecimal to receive the constant value.
//   precision - Number of fractional decimal digits requested.
// Returns:
//   1 on success, 0 on failure.
int math_const_pi(BigDecimal *out, int precision);

// Computes or retrieves Euler's number e (2.7182818284...) rounded to target precision.
// Parameters:
//   out       - Target BigDecimal to receive the constant value.
//   precision - Number of fractional decimal digits requested.
// Returns:
//   1 on success, 0 on failure.
int math_const_e(BigDecimal *out, int precision);

// Computes or retrieves natural logarithm of 10, ln(10) (2.3025850929...),
// rounded to target precision.
// Parameters:
//   out       - Target BigDecimal to receive the constant value.
//   precision - Number of fractional decimal digits requested.
// Returns:
//   1 on success, 0 on failure.
int math_const_ln10(BigDecimal *out, int precision);

// ============================================================================
// Scientific Functions on BigDecimal (Evaluated to target precision)
// ============================================================================

// Computes the principal square root of non-negative decimal x: out = sqrt(x)
// Implemented via arbitrary-precision Newton-Raphson iteration:
//   y_{k+1} = 0.5 * (y_k + x / y_k)
// Parameters:
//   out       - Target BigDecimal for the square root result.
//   x         - Non-negative input radicand (x >= 0).
//   precision - Fractional decimal precision of the result.
// Returns:
//   1 on success, 0 on failure or negative input (domain error).
int fn_sqrt(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the exponential function: out = exp(x) = e^x
// Implemented via range splitting x = m + f (where m is integer, 0 <= f < 1),
// binary exponentiation for e^m, and Taylor series for e^f:
//   sum_{k=0}^N [ f^k / k! ]
// Parameters:
//   out       - Target BigDecimal for the exponential result.
//   x         - Exponent value.
//   precision - Fractional decimal precision of the result.
// Returns:
//   1 on success, 0 on failure or overflow.
int fn_exp(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the natural logarithm: out = ln(x) for x > 0
// Implemented via range reduction x = y * 10^k (0.5 <= y < 5.0) and
// transformed hyperbolic arctangent series:
//   ln(x) = k * ln(10) + 2 * sum_{k=0}^N [ (1 / (2k + 1)) * ((y - 1)/(y + 1))^(2k + 1) ]
// Parameters:
//   out       - Target BigDecimal for the logarithm result.
//   x         - Strictly positive input value (x > 0).
//   precision - Fractional decimal precision of the result.
// Returns:
//   1 on success, 0 on failure or non-positive input (domain error).
int fn_ln(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the trigonometric sine of an angle given in radians: out = sin(x)
// Implemented via range reduction modulo 2*pi and truncated Maclaurin series:
//   sin(x) = sum_{k=0}^N [ (-1)^k * x^(2k + 1) / (2k + 1)! ]
// Parameters:
//   out       - Target BigDecimal for the sine result.
//   x         - Angle in radians.
//   precision - Fractional decimal precision of the result.
// Returns:
//   1 on success, 0 on failure.
int fn_sin(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the trigonometric cosine of an angle given in radians: out = cos(x)
// Implemented via range reduction modulo 2*pi and truncated Maclaurin series:
//   cos(x) = sum_{k=0}^N [ (-1)^k * x^(2k) / (2k)! ]
// Parameters:
//   out       - Target BigDecimal for the cosine result.
//   x         - Angle in radians.
//   precision - Fractional decimal precision of the result.
// Returns:
//   1 on success, 0 on failure.
int fn_cos(BigDecimal *out, const BigDecimal *x, int precision);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H

