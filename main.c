#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"

// ============================================================================
// File: main.c
// Purpose: Main command-line entry point for the Arbitrary-Precision Calculator.
// Role:
//   Inspects command-line arguments to determine whether to run in interactive
//   REPL mode, display usage/help information, or evaluate a one-shot expression
//   directly from the shell and exit.
// ============================================================================

// Display usage guidelines, supported command flags, and sample expressions to stdout.
static void print_usage(const char *prog_name) {
    // Print banner and command-line invocation syntax
    printf("Arbitrary-Precision Calculator\n");
    printf("Usage:\n");
    printf("  %s                   Run interactive REPL\n", prog_name);
    printf("  %s \"<expression>\"    Evaluate an expression and exit\n", prog_name);
    printf("  %s -e \"<expression>\" Evaluate an expression and exit\n", prog_name);
    printf("  %s -h, --help        Display this help message\n\n", prog_name);
    
    // Provide illustrative examples showing different mathematical features
    printf("Examples:\n");
    printf("  %s \"1 + 2 * 3^2 - (40/8)\"\n", prog_name);
    printf("  %s \"100! / 99!\"\n", prog_name);
    printf("  %s \"gcd(48, 18)\"\n", prog_name);
}

int main(int argc, char *argv[]) {
    // Case 1: No arguments provided -> Launch interactive Read-Eval-Print Loop (REPL)
    if (argc == 1) {
        // Starts the interactive prompt where users can type expressions line-by-line
        return run_repl();
    }

    // Case 2: User requested help flags (-h or --help)
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        // Output help text and return success (0)
        print_usage(argv[0]);
        return 0;
    }

    // Case 3: Flag '-e' specified for explicit expression evaluation
    if (strcmp(argv[1], "-e") == 0) {
        // Ensure that an expression argument was actually provided after '-e'
        if (argc < 3) {
            fprintf(stderr, "Error: Option '-e' requires an expression argument.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        // Evaluate the provided expression string and return exit code
        return run_expression(argv[2]);
    }

    // Case 4: A single expression passed directly without flags, e.g., ./calculator "2 + 2"
    if (argc == 2) {
        return run_expression(argv[1]);
    }

    // Case 5: Expression passed as multiple unquoted CLI arguments (e.g., ./calculator 2 + 3 * 4)
    // We compute total length needed to concatenate all arguments with space delimiters
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        // Length of argument + 1 for separating space or null terminator
        total_len += strlen(argv[i]) + 1;
    }

    // Allocate memory for the joined expression string
    char *combined = (char *)malloc(total_len + 1);
    if (!combined) {
        // Allocation failure guard
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    // Initialize combined string as empty
    combined[0] = '\0';
    // Append each argument separated by a single whitespace character
    for (int i = 1; i < argc; i++) {
        strcat(combined, argv[i]);
        if (i < argc - 1) {
            strcat(combined, " ");
        }
    }

    // Evaluate the reconstructed expression string
    int ret = run_expression(combined);
    
    // Free the dynamically allocated combined string buffer
    free(combined);
    
    // Return the execution result status code (0 for success, non-zero for error)
    return ret;
}


