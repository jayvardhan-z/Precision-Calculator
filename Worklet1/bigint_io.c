#include "bigint_io.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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