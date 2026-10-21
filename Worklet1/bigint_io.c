#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// File: bigint_io.c
// Purpose: I/O Utilities & Lossless String <-> BigInteger Conversions.
// Role:
//   Acts as the input/output bridge between the human/text domain and the
//   internal arithmetic core (Worklets 2 & 3).
//
// Responsibilities:
//   1. Input Handling: Dynamically reads arbitrary-length input lines from
//      streams (stdin/files) and cleans whitespace before lexing/parsing.
//   2. Deserialization: Converts decimal number strings into Base-10^9 BigInteger
//      structures (packed 9 digits per limb) for the lexer and parser.
//   3. Serialization: Formats internal BigInteger results back into decimal
//      strings for displaying outputs in the REPL and test runner.
// ============================================================================

// Trims leading and trailing whitespace characters in-place from a string.
// Used in: The REPL input loop and expression pre-validator to clean raw user commands before lexing.
void trim_whitespace(char *str) {
    // Return immediately if string pointer is null or string is empty
    if (!str || !*str)
        return;

    // Step 1: Scan backwards from string end to find the last non-whitespace character
    int end = strlen(str) - 1;
    while (end >= 0 && isspace((unsigned char)str[end])) {
        end--;
    }
    // Null-terminate immediately after the last non-space character
    str[end + 1] = '\0';

    // Step 2: Scan forwards from string start to find the first non-whitespace character
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }

    // Step 3: Shift remaining valid characters to the left if leading whitespace existed
    if (start > 0) {
        int i = 0;
        while (start <= end) {
            str[i++] = str[start++];
        }
        // Place new null terminator at shifted end
        str[i] = '\0';
    }
}

// Reads an arbitrary-length line of text from a stream into dynamically allocated memory.
// Used in: The interactive REPL (stdin) and automated test runner (file streams) to read expressions.
char *read_input_line(FILE *stream) {
    if (!stream)
        return NULL;

    // Initial buffer capacity: 128 bytes
    int cap = 128;
    int len = 0;
    char *line = malloc(cap);
    if (!line)
        return NULL;

    int c;
    // Read characters one-by-one until newline or EOF
    while ((c = fgetc(stream)) != EOF && c != '\n') {
        // If buffer is full (leaving 1 byte for null terminator), double its capacity
        if (len + 1 >= cap) {
            cap *= 2;
            char *temp = realloc(line, cap);
            if (!temp) {
                // If reallocation fails, free buffer and return NULL
                free(line);
                return NULL;
            }
            line = temp;
        }
        // Append character to buffer
        line[len++] = (char)c;
    }

    // If stream reached EOF without reading any characters, signal EOF by returning NULL
    if (len == 0 && c == EOF) {
        free(line);
        return NULL;
    }

    // Null-terminate the string buffer
    line[len] = '\0';
    return line;
}

// Parses a decimal string with optional signs into a Base-10^9 BigInteger (9 digits per limb).
// Used in: The Lexer/Tokenizer (TK_NUMBER) and parser to convert string numeric literals into BigInteger operands.
int bigint_from_string(BigInteger *a, const char *str) {
    if (!a || !str)
        return 0;

    // Step 1: Skip leading whitespace characters
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }

    // Step 2: Parse optional positive or negative sign prefix
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Step 3: Skip leading zeroes (e.g., "000123" -> "123")
    int had_zeros = 0;
    while (*str == '0') {
        had_zeros = 1;
        str++;
    }

    // Step 4: Handle string terminating after zeros or bare sign
    if (!*str) {
        if (had_zeros) {
            // String was "0", "+000", "-0", etc. -> Result is canonical zero
            bigint_clear(a);
            return 1;
        }
        // Bare "+" or "-" with no digits is invalid syntax
        return 0;
    }

    // Step 5: Verify all remaining characters are decimal digits [0-9]
    const char *digits = str;
    while (*str) {
        if (!isdigit((unsigned char)*str))
            return 0; // Invalid character found
        str++;
    }

    // Step 6: Calculate number of digits and required Base-10^9 limbs
    // Each limb holds 9 decimal digits: num_limbs = ceil(len / 9)
    int len = str - digits;
    int num_limbs = (len + BASE_DIGITS - 1) / BASE_DIGITS;

    // Reserve sufficient capacity for limbs array
    if (!bigint_reserve(a, num_limbs))
        return 0;

    // Reset integer structure and assign target limb count and sign
    bigint_clear(a);
    a->size = num_limbs;
    a->sign = sign;

    // Step 7: Parse 9-digit chunks from right (least-significant) to left (most-significant)
    int end = len;
    int limb = 0;
    while (end > 0) {
        // Determine start boundary for current 9-digit limb
        int start = (end >= BASE_DIGITS) ? (end - BASE_DIGITS) : 0;
        uint32_t val = 0;
        
        // Convert the up-to-9 ASCII digit characters into a binary integer
        for (int i = start; i < end; i++) {
            val = val * 10 + (digits[i] - '0');
        }
        
        // Store limb in little-endian order (digits[0] = least significant)
        a->digits[limb++] = val;
        // Advance chunk boundary
        end = start;
    }

    // Step 8: Normalize to enforce canonical invariants (e.g., stripping accidental zeros)
    bigint_normalize(a);
    return 1;
}

// Converts a Base-10^9 BigInteger into a human-readable decimal string representation.
// Used in: Output formatting, REPL result printing, and variable assignment display.
char *bigint_to_string(const BigInteger *a) {
    if (!a)
        return NULL;

    // Case 1: Check for zero value -> Render as "0"
    if (a->size == 0 || (a->size == 1 && a->digits[0] == 0)) {
        char *str = malloc(2);
        if (!str)
            return NULL;
        strcpy(str, "0");
        return str;
    }

    // Case 2: Non-zero value
    // Allocate buffer: 1 sign + 9 digits per limb + 1 null terminator
    int max_len = a->size * BASE_DIGITS + 2;
    char *buf = malloc(max_len);
    if (!buf)
        return NULL;

    int pos = 0;
    // Emit negative sign if number is negative
    if (a->sign < 0) {
        buf[pos++] = '-';
    }

    // Print the most significant limb without leading zero padding (%u)
    pos += sprintf(buf + pos, "%u", a->digits[a->size - 1]);

    // Print all remaining lower-order limbs with zero padding to exactly 9 digits (%09u)
    for (int i = (int)a->size - 2; i >= 0; i--) {
        pos += sprintf(buf + pos, "%09u", a->digits[i]);
    }

    return buf;
}