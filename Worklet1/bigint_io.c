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