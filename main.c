#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"

static void print_usage(const char *prog_name) {
    printf("Arbitrary-Precision Calculator\n");
    printf("Usage:\n");
    printf("  %s                   Run interactive REPL\n", prog_name);
    printf("  %s \"<expression>\"    Evaluate an expression and exit\n", prog_name);
    printf("  %s -e \"<expression>\" Evaluate an expression and exit\n", prog_name);
    printf("  %s -h, --help        Display this help message\n\n", prog_name);
    printf("Examples:\n");
    printf("  %s \"1 + 2 * 3^2 - (40/8)\"\n", prog_name);
    printf("  %s \"100! / 99!\"\n", prog_name);
    printf("  %s \"gcd(48, 18)\"\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return run_repl();
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Option '-e' requires an expression argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        return run_expression(argv[2]);
    }

    // If expression is passed in a single string or split across arguments
    if (argc == 2) {
        return run_expression(argv[1]);
    }

    // Join remaining arguments into a single expression string
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        total_len += strlen(argv[i]) + 1;
    }

    char *combined = (char *)malloc(total_len + 1);
    if (!combined) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    combined[0] = '\0';
    for (int i = 1; i < argc; i++) {
        strcat(combined, argv[i]);
        if (i < argc - 1) {
            strcat(combined, " ");
        }
    }

    int ret = run_expression(combined);
    free(combined);
    return ret;
}

