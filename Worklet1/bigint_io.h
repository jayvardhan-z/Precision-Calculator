#ifndef BIGINT_IO_H
#define BIGINT_IO_H

#include <stdio.h>
#include "bigint.h"

// ============================================================================
// File: bigint_io.h
// Purpose: Prototypes for arbitrary-precision integer string I/O and stream reading.
// Role:
//   Connects human-readable text representations with internal Base-10^9
//   limb representations, providing lossless bidirectional conversions and
//   safe stream input reading with dynamic buffer growth.
// ============================================================================

// ----------------------------------------------------------------------------
// Lossless String <-> BigInteger Conversions
// ----------------------------------------------------------------------------

// Parses a decimal string (with optional +/- sign and leading zeros) into a Base-10^9 BigInteger
// Returns 1 on success, 0 on failure (invalid syntax, illegal characters, memory failure)
int bigint_from_string(BigInteger *a, const char *str);

// Serializes a Base-10^9 BigInteger into a newly allocated null-terminated decimal string
// Formats negative sign if negative, and zero-pads all limbs except the most significant limb
// Returns heap-allocated char* that caller must free(), or NULL on failure
char *bigint_to_string(const BigInteger *a);

// ----------------------------------------------------------------------------
// String & Stream Utilities
// ----------------------------------------------------------------------------

// Trims leading and trailing whitespace characters in-place from a null-terminated string
void trim_whitespace(char *str);

// Reads a full line of unbounded length from the given stream until '\n' or EOF
// Dynamically resizes heap buffer to prevent buffer overflows
// Returns heap-allocated string that caller must free(), or NULL on EOF/error
char *read_input_line(FILE *stream);

#endif /* BIGINT_IO_H */