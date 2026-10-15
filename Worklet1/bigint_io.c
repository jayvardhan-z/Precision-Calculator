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
