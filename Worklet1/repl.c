#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "bigint_io.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"

// ============================================================================
// File: repl.c
// Purpose: Interactive Read-Eval-Print Loop (REPL) and execution coordinator.
// Role:
//   Coordinates input reading, internal command interpretation, syntax
//   validation, tokenization, AST parsing, AST post-order evaluation,
//   high-resolution execution timing, and formatted result presentation.
// ============================================================================

// Encapsulates the runtime state of the interactive calculator session
typedef struct {
    int         running; // Loop control flag (1 = keep running, 0 = exit)
    SymbolTable vars;    // Persistent symbol table holding variables and constants
    EvalContext ctx;     // Evaluation context (precision, angle mode, rounding)
} ReplState;

// Initializes REPL session state with default settings and preloaded constants
static void repl_state_init(ReplState *repl) {
    repl->running = 1;
    // Initialize persistent variable table
    symtab_init(&repl->vars);
    // Bind symbol table to evaluation context
    eval_context_init(&repl->ctx, &repl->vars);
    // Set default configuration: 20 decimal places, radians, round-half-up
    repl->ctx.precision = 20;
    repl->ctx.angle_mode = ANGLE_RADIAN;
    repl->ctx.rounding = ROUND_HALF_UP;
    // Precompute mathematical constants (pi, e, i, ans) to target precision
    symtab_preload_constants(&repl->vars, repl->ctx.precision);
}

// Releases all allocated memory associated with the REPL session
static void repl_state_free(ReplState *repl) {
    symtab_free(&repl->vars);
}

// Displays the interactive prompt string to stdout and flushes the output stream
static void display_prompt(void) {
    printf("> ");
    fflush(stdout);
}

// Formats and displays an error message with consistent indentation
static void print_error(const char *msg) {
    printf("  Error: %s\n\n", msg);
}

// Inspects and handles internal REPL meta-commands (exit, help, mode, precision, vars)
// Returns 1 if the input line was a meta-command, 0 if it should be evaluated as an expression
static int handle_repl_command(ReplState *repl, const char *line) {
    // Empty line: treat as processed command, no evaluation needed
    if (line[0] == '\0') return 1;

    // Command: "exit", "quit", or "q" -> Terminate REPL loop
    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
        repl->running = 0;
        return 1;
    }

    // Command: "help" or "?" -> Display available commands and mathematical syntax
    if (strcmp(line, "help") == 0 || strcmp(line, "?") == 0) {
        printf("\n=== Commands ===\n");
        printf("  exit, quit         Exit the calculator\n");
        printf("  help               Show this message\n");
        printf("  mode [rad|deg]     Set or toggle angle mode\n");
        printf("  precision <n>      Set decimal precision\n");
        printf("  vars               Show stored variables\n");
        printf("=== Syntax ===\n");
        printf("  + - * / %% ^ !     arithmetic, power, modulo, factorial\n");
        printf("  sin cos exp ln     scientific functions (Taylor series)\n");
        printf("  sqrt abs gcd       square root, absolute value, GCD\n");
        printf("  x = <expr>         assign a variable\n");
        printf("  pi, e, ans         constants and the previous result\n\n");
        return 1;
    }

    // Command: "precision" alone -> Print current decimal precision
    if (strcmp(line, "precision") == 0) {
        printf("  [Current precision: %d]\n\n", repl->ctx.precision);
        return 1;
    }

    // Command: "precision <n>" -> Update decimal precision and recompute constants
    if (strncmp(line, "precision ", 10) == 0) {
        int p = atoi(line + 10);
        // Constrain precision between 0 and 2000 decimal places
        if (p >= 0 && p <= 2000) {
            repl->ctx.precision = p;
            symtab_preload_constants(&repl->vars, p); // Recompute pi and e with new precision
            printf("  [Precision set to %d decimal places]\n\n", p);
        } else {
            printf("  [Error: precision must be between 0 and 2000]\n\n");
        }
        return 1;
    }

    // Command: "mode" -> Toggle or set trigonometric angle mode (degrees vs radians)
    if (strcmp(line, "mode") == 0 || strncmp(line, "mode ", 5) == 0) {
        const char *arg = line + 4;
        while (*arg == ' ') arg++;

        if (strcmp(arg, "deg") == 0 || strcmp(arg, "degree") == 0) {
            repl->ctx.angle_mode = ANGLE_DEGREE;
        } else if (strcmp(arg, "rad") == 0 || strcmp(arg, "radian") == 0) {
            repl->ctx.angle_mode = ANGLE_RADIAN;
        } else {
            // Toggle between Radian and Degree if no argument specified
            if (repl->ctx.angle_mode == ANGLE_RADIAN)
                repl->ctx.angle_mode = ANGLE_DEGREE;
            else
                repl->ctx.angle_mode = ANGLE_RADIAN;
        }

        if (repl->ctx.angle_mode == ANGLE_RADIAN)
            printf("  [Angle mode is now RADIAN]\n\n");
        else
            printf("  [Angle mode is now DEGREE]\n\n");
        return 1;
    }

    // Command: "vars" -> Print all registered symbols and their current values
    if (strcmp(line, "vars") == 0) {
        printf("\n=== Variables ===\n");
        for (size_t i = 0; i < repl->vars.count; i++) {
            char *s = value_to_string(&repl->vars.symbols[i].value);
            printf("  %s = %s\n", repl->vars.symbols[i].name, s);
            free(s);
        }
        printf("\n");
        return 1;
    }

    return 0; // Not a meta-command; line is a mathematical expression
}

// Parses assignment syntax: "<variable_name> = <expression>"
// Returns heap-allocated variable name string if assignment, or NULL if not
static char *parse_assignment(char *line) {
    char *eq = strchr(line, '=');
    if (!eq) return NULL; // No '=' character present

    // Ensure the variable name starts with an alphabetic character or underscore
    char *start = line;
    while (*start == ' ') start++;
    if (!((*start >= 'a' && *start <= 'z') ||
          (*start >= 'A' && *start <= 'Z') ||
          *start == '_')) return NULL;

    // Scan until end of identifier token
    char *end = start;
    while ((*end >= 'a' && *end <= 'z') ||
           (*end >= 'A' && *end <= 'Z') ||
           (*end >= '0' && *end <= '9') ||
           *end == '_') end++;

    // Only whitespace is allowed between variable name and '='
    char *after = end;
    while (*after == ' ') after++;
    if (after != eq) return NULL;

    // Duplicate variable name
    int len = end - start;
    char *name = malloc(len + 1);
    memcpy(name, start, len);
    name[len] = '\0';
    return name;
}

// Executes a full cycle for a single input line: pre-validates, tokenizes, parses, evaluates, times, and prints
int execute_repl_cycle(ReplState *repl, const char *raw_line) {
    // Create working copy so trimming does not mutate the caller's buffer
    char *line = strdup(raw_line);
    trim_whitespace(line);

    // Process meta-commands (exit, help, mode, precision, vars)
    if (handle_repl_command(repl, line)) {
        free(line);
        return 1;
    }

    // Initialize pipeline structures
    TokenList tokens;
    ParsedExpression pe;
    ParseError perr;
    Value result;
    size_t err_pos = 0;
    int ok = 0;

    token_list_init(&tokens);
    parsed_expression_init(&pe);
    value_init(&result, VAL_INT);

    // Check if the input line is an assignment: "name = expr"
    char *expr = line;
    char *var_name = parse_assignment(line);
    if (var_name) {
        // Strip everything up to '=' so expr holds only the right-hand mathematical expression
        expr = strchr(line, '=') + 1;
        trim_whitespace(expr);
    }

    // Step 1: Pre-validation of characters and parentheses
    if (!validate_expression(expr, &err_pos)) {
        print_error("Incorrect expression");
    }
    // Step 2: Lexical analysis (tokenization)
    else if (!lex_tokenize(expr, &tokens, &perr)) {
        print_error("Incorrect expression");
    }
    // Step 3: Recursive-descent AST construction
    else if (!parse_expression_tree(&tokens, &pe)) {
        print_error("Incorrect expression");
    }
    // Step 4: Semantic evaluation and timing
    else {
        // Reset evaluation context errors and recursion guard
        repl->ctx.depth = 0;
        repl->ctx.error.code = ERR_NONE;
        repl->ctx.error.pos = 0;
        repl->ctx.error.message[0] = '\0';

        // High-resolution interval timing around AST evaluation
        Timer timer;
        timer_start(&timer);
        int eval_ok = eval_node(pe.root, &repl->ctx, &result);
        timer_stop(&timer);

        if (!eval_ok) {
            // Evaluation failed (e.g. division by zero, domain error) -> Print specific error message
            print_error(repl->ctx.error.message);
        } else {
            // Convert calculated Value to human-readable string and display with elapsed milliseconds
            char *res_str = value_to_string(&result);
            if (var_name)
                printf("  %s = %s  [%.4f ms]\n\n", var_name, res_str, timer_elapsed_ms(&timer));
            else
                printf("  = %s  [%.4f ms]\n\n", res_str, timer_elapsed_ms(&timer));
            free(res_str);

            // Automatically store result in special variable "ans" for result chaining
            symtab_set(&repl->vars, "ans", &result);
            // If user assigned to a variable name, store result in that symbol
            if (var_name) symtab_set(&repl->vars, var_name, &result);
            ok = 1;
        }
    }

    // Free all resources allocated for this evaluation cycle
    free(var_name);
    value_free(&result);
    parsed_expression_free(&pe);
    token_list_free(&tokens);
    free(line);
    return ok;
}

// Displays welcome banner upon REPL startup
void display_menu(void) {
    printf("=== Arbitrary-Precision Calculator ===\n");
    printf("Type 'help' for commands, 'exit' or Ctrl+D to quit.\n\n");
}

// Main interactive loop reading expressions from stdin until exit
int run_repl(void) {
    ReplState repl;
    repl_state_init(&repl);

    display_menu();

    // Loop until user enters exit/quit command or sends EOF (Ctrl+D)
    while (repl.running) {
        display_prompt();
        char *line = read_input_line(stdin);
        if (!line) { // End-of-file reached (Ctrl+D)
            printf("\nExiting.\n");
            break;
        }
        execute_repl_cycle(&repl, line);
        free(line);
    }

    repl_state_free(&repl);
    return 0;
}

// Evaluates a single expression string non-interactively and cleans up
int run_expression(const char *expr) {
    if (!expr) return 1;
    ReplState repl;
    repl_state_init(&repl);
    int ok = execute_repl_cycle(&repl, expr);
    repl_state_free(&repl);
    return ok ? 0 : 1;
}