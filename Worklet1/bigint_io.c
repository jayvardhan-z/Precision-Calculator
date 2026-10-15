#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/**
 * =============================================================================
 * File: bigint_io.c
 * Purpose: I/O Utilities & Lossless String <-> BigInteger Conversions
 * 
 * Role in Project:
 *   Acts as the input/output bridge between the human/text domain and the
 *   internal arithmetic core (Worklets 2 & 3).
 *
 * Responsibilities:
 *   1. Input Handling: Dynamically reads arbitrary-length input lines from
 *      streams (stdin/files) and cleans whitespace before lexing/parsing.
 *   2. Deserialization: Converts decimal number strings into Base-10^9 BigInteger
 *      structures (packed 9 digits per limb) for the lexer and parser.
 *   3. Serialization: Formats internal BigInteger results back into decimal
 *      strings for displaying outputs in the REPL and test runner.
 * =============================================================================
 */




/**
 * Trims leading and trailing whitespace characters in-place from a string.
 * Used in: The REPL input loop and expression pre-validator to clean raw user commands before lexing.
 */
void trim_whitespace(char *str)
{
    if (!str || !*str)
        return;

    // Trim trailing
    int end = strlen(str) - 1;
    while (end >= 0 && isspace(str[end]))
    {
        end--;
    }
    str[end + 1] = '\0';

    // Find first non-space
    int start = 0;
    while (str[start] && isspace(str[start]))
    {
        start++;
    }

    // Shift left if there were leading spaces
    if (start > 0)
    {
        int i = 0;
        while (start <= end)
        {
            str[i++] = str[start++];
        }
        str[i] = '\0';
    }
}


/**
 * Reads an arbitrary-length line of text from a stream into dynamically allocated memory.
 * Used in: The interactive REPL (stdin) and automated test runner (file streams) to read expressions.
 */
char *read_input_line(FILE *stream)
{
    if (!stream)
        return NULL;

    int cap = 128;
    int len = 0;
    char *line = malloc(cap);
    if (!line)
        return NULL;

    int c;
    while ((c = fgetc(stream)) != EOF && c != '\n')
    {
        // Resize buffer if full (leaving space for '\0')
        if (len + 1 >= cap)
        {
            cap *= 2;
            char *temp = realloc(line, cap);
            if (!temp)
            {
                free(line);
                return NULL;
            }
            line = temp;
        }
        line[len++] = c;
    }

    // Return NULL if EOF was reached with zero input
    if (len == 0 && c == EOF)
    {
        free(line);
        return NULL;
    }

    line[len] = '\0';
    return line;
}


/**
 * Parses a decimal string with optional signs into a Base-10^9 BigInteger (9 digits per limb).
 * Used in: The Lexer/Tokenizer (TOKEN_NUMBER) and parser to convert string numeric literals into BigInteger operands.
 */
int bigint_from_string(BigInteger *a, const char *str)
{
    if (!a || !str)
        return 0;

    // Skip leading spaces
    while (*str && isspace(*str))
    {
        str++;
    }

    // Optional sign
    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    // Skip leading zeros
    int had_zeros = 0;
    while (*str == '0')
    {
        had_zeros = 1;
        str++;
    }

    // Handle zero or bare signs
    if (!*str)
    {
        if (had_zeros)
        {
            bigint_clear(a);
            return 1;
        }
        return 0; // Bare "+" or "-" with no digits
    }

    // Validate digits and calculate length
    const char *digits = str;
    while (*str)
    {
        if (!isdigit(*str))
            return 0;
        str++;
    }

    int len = str - digits;
    int num_limbs = (len + BASE_DIGITS - 1) / BASE_DIGITS;

    if (!bigint_reserve(a, num_limbs))
        return 0;

    bigint_clear(a);
    a->size = num_limbs;
    a->sign = sign;

    // Parse 9-digit chunks from right to left
    int end = len;
    int limb = 0;
    while (end > 0)
    {
        int start = (end >= BASE_DIGITS) ? (end - BASE_DIGITS) : 0;
        uint32_t val = 0;
        for (int i = start; i < end; i++)
        {
            val = val * 10 + (digits[i] - '0');
        }
        a->digits[limb++] = val;
        end = start;
    }

    bigint_normalize(a);
    return 1;
}