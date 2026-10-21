// ============================================================================
// File: Worklet3/functions.h
// Description: Arbitrary-precision transcendental, scientific, and mathematical
//              function declarations operating on BigDecimal objects.
//
// Overview:
// Computations on BigDecimal values cannot rely on standard IEEE 754 <math.h>
// functions because standard doubles only provide 53 bits (~15-17 decimal digits)
// of significand precision. This module defines the interfaces for computing
// mathematical constants (pi, e, ln10) and transcendental functions (sqrt, exp,
// ln, sin, cos) to arbitrary user-specified decimal precisions.
//
// Key Algorithmic Principles:
// 1. Guard Digits: Intermediate calculations require extra digits (typically
//    precision + 8) to mitigate round-off errors and catastrophic cancellation.
// 2. Range Reduction: Domain transformations are applied before series evaluation
//    to ensure fast convergence within regions of rapid Taylor series decay.
// 3. Newton-Raphson Iteration: Quadratic convergence doubling accurate digits per step.
// ============================================================================

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// Core arbitrary-precision fixed-point decimal definitions
#include "decimal.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Mathematical Constants
// ============================================================================

// Computes the mathematical constant pi (3.1415926535...) to the requested
// decimal precision.
//
// Algorithmic Approaches:
// - Machin's Formula: pi/4 = 4*arctan(1/5) - arctan(1/239)
// - Chudnovsky Algorithm: Hypergeometric series producing ~14 digits per term.
//
// Parameters:
//   out       - Target BigDecimal to receive the computed constant.
//   precision - Number of fractional decimal digits of accuracy required.
// Returns:
//   1 on success, 0 on failure or out-of-memory.
int math_const_pi(BigDecimal *out, int precision);

// Computes Euler's number e (2.7182818284...) to the requested decimal precision.
//
// Algorithmic Approach:
// - Standard Taylor series expansion: e = sum_{k=0}^{N} (1 / k!)
// - Truncation condition: Term magnitude 1/k! drops below 10^(-precision).
//
// Parameters:
//   out       - Target BigDecimal to receive the computed constant.
//   precision - Number of fractional decimal digits of accuracy required.
// Returns:
//   1 on success, 0 on failure or out-of-memory.
int math_const_e(BigDecimal *out, int precision);

// Computes the natural logarithm of 10, ln(10) (2.3025850929...), to the
// requested decimal precision.
//
// Usage:
// - Fundamental constant for base conversion and decimal range reduction
//   in the natural logarithm algorithm: ln(x * 10^k) = k * ln(10) + ln(x).
//
// Parameters:
//   out       - Target BigDecimal to receive the computed constant.
//   precision - Number of fractional decimal digits of accuracy required.
// Returns:
//   1 on success, 0 on failure or out-of-memory.
int math_const_ln10(BigDecimal *out, int precision);

// ============================================================================
// Scientific Functions on BigDecimal
// ============================================================================

// Computes the principal square root of an arbitrary-precision decimal:
//   out = sqrt(x)
//
// Algorithmic Approach:
// - Arbitrary-precision Newton-Raphson iteration:
//     y_{k+1} = 0.5 * (y_k + x / y_k)
// - Initial estimate y_0 is derived from double conversion or magnitude scaling.
// - Each iteration quadratically doubles the count of correct significant digits.
//
// Domain Constraints:
//   x >= 0. Negative arguments produce a domain error (returns 0).
//
// Parameters:
//   out       - Target BigDecimal for the square root result.
//   x         - Non-negative input radicand.
//   precision - Desired fractional decimal precision.
// Returns:
//   1 on success, 0 on failure or domain violation.
int fn_sqrt(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the exponential function:
//   out = exp(x) = e^x
//
// Algorithmic Approach:
// - Range reduction: Split x into an integer part m and fractional part f:
//     x = m + f, where 0 <= f < 1
// - Compute e^m via repeated squaring of e, and compute e^f via Taylor series:
//     e^f = sum_{k=0}^N (f^k / k!)
// - Combine: exp(x) = e^m * e^f
//
// Parameters:
//   out       - Target BigDecimal for the exponential result.
//   x         - Exponent value.
//   precision - Target fractional precision.
// Returns:
//   1 on success, 0 on calculation failure.
int fn_exp(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the natural logarithm:
//   out = ln(x)
//
// Algorithmic Approach:
// - Range reduction via scientific exponent separation:
//     x = y * 10^k, with y scaled to interval [1, 10)
//     ln(x) = k * ln(10) + ln(y)
// - Rapidly converging transformation: let t = (y - 1) / (y + 1), then:
//     ln(y) = 2 * sum_{k=0}^N [ 1 / (2k + 1) * t^(2k + 1) ]
//
// Domain Constraints:
//   x > 0. Values of x <= 0 produce a domain error (returns 0).
//
// Parameters:
//   out       - Target BigDecimal for the natural log result.
//   x         - Strictly positive input value.
//   precision - Target fractional precision.
// Returns:
//   1 on success, 0 on failure or domain error.
int fn_ln(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the sine of an angle given in radians:
//   out = sin(x)
//
// Algorithmic Approach:
// - Range reduction: Reduce input x modulo 2*pi to the interval [-pi, pi]
//   (and optionally to [-pi/4, pi/4] using symmetry identities).
// - Taylor / Maclaurin series expansion:
//     sin(x) = sum_{k=0}^N [ (-1)^k * x^(2k + 1) / (2k + 1)! ]
//
// Parameters:
//   out       - Target BigDecimal for the sine result.
//   x         - Angle in radians.
//   precision - Target fractional precision.
// Returns:
//   1 on success, 0 on failure.
int fn_sin(BigDecimal *out, const BigDecimal *x, int precision);

// Computes the cosine of an angle given in radians:
//   out = cos(x)
//
// Algorithmic Approach:
// - Range reduction: Reduce input x modulo 2*pi to the interval [0, pi/2]
//   via standard trigonometric symmetries.
// - Taylor / Maclaurin series expansion:
//     cos(x) = sum_{k=0}^N [ (-1)^k * x^(2k) / (2k)! ]
//
// Parameters:
//   out       - Target BigDecimal for the cosine result.
//   x         - Angle in radians.
//   precision - Target fractional precision.
// Returns:
//   1 on success, 0 on failure.
int fn_cos(BigDecimal *out, const BigDecimal *x, int precision);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H


