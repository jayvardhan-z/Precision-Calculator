#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "timer.h"
#include "bigint_io.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"

typedef struct {
    int running;
    SymbolTable vars;
    EvalContext ctx;
} ReplState;

static void repl_state_init(ReplState *repl) {
    repl->running = 1;
    symtab_init(&repl->vars);
    eval_context_init(&repl->ctx, &repl->vars);
    repl->ctx.precision = 20;
    repl->ctx.angle_mode = ANGLE_RADIAN;
    repl->ctx.rounding = ROUND_HALF_UP;
    symtab_preload_constants(&repl->vars, repl->ctx.precision);
}

static void repl_state_free(ReplState *repl) {
    symtab_free(&repl->vars);
}

static void display_prompt(void) {
    printf("> ");
    fflush(stdout);
}

/* if line starts with the command name, return a pointer to its argument */
static const char *command_args(const char *line, const char *name) {
    size_t n = strlen(name);
    if (strncmp(line, name, n) != 0) return NULL;
    if (line[n] != '\0' && !isspace((unsigned char)line[n])) return NULL;
    const char *p = line + n;
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* returns 1 if the line was a command, 0 if it should be evaluated */
static int handle_repl_command(ReplState *repl, const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line) return 1;   /* empty line, nothing to do */

    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
        repl->running = 0;
        return 1;
    }
    if (strcmp(line, "help") == 0 || strcmp(line, "?") == 0) {
        printf("\n=== Commands ===\n");
        printf("  exit, quit         Exit the calculator\n");
        printf("  help               Show this message\n");
        printf("  mode [rad|deg]     Set or toggle angle mode (now: %s)\n",
               repl->ctx.angle_mode == ANGLE_RADIAN ? "radian" : "degree");
        printf("  precision <n>      Set decimal precision (now: %d)\n", repl->ctx.precision);
        printf("  vars               Show stored variables\n");
        printf("=== Syntax ===\n");
        printf("  + - * / %% ^ !     arithmetic, power, modulo, factorial\n");
        printf("  sin cos exp ln     scientific functions (Taylor series)\n");
        printf("  sqrt abs gcd       square root, absolute value, GCD\n");
        printf("  x = <expr>         assign a variable\n");
        printf("  pi, e, ans         constants and the previous result\n\n");
        return 1;
    }

    const char *args = command_args(line, "precision");
    if (args) {
        if (*args) {
            int p = atoi(args);
            if (p >= 0 && p <= 2000) {
                repl->ctx.precision = p;
                symtab_preload_constants(&repl->vars, p);  /* recompute pi, e */
                printf("  [Precision set to %d decimal places]\n\n", p);
            } else {
                printf("  [Error: precision must be between 0 and 2000]\n\n");
            }
        } else {
            printf("  [Current precision: %d]\n\n", repl->ctx.precision);
        }
        return 1;
    }

    args = command_args(line, "mode");
    if (args) {
        if (strcmp(args, "deg") == 0 || strcmp(args, "degree") == 0) {
            repl->ctx.angle_mode = ANGLE_DEGREE;
        } else if (strcmp(args, "rad") == 0 || strcmp(args, "radian") == 0) {
            repl->ctx.angle_mode = ANGLE_RADIAN;
        } else {
            /* no argument, so just toggle */
            repl->ctx.angle_mode = (repl->ctx.angle_mode == ANGLE_RADIAN)
                                       ? ANGLE_DEGREE : ANGLE_RADIAN;
        }
        printf("  [Angle mode is now %s]\n\n",
               repl->ctx.angle_mode == ANGLE_RADIAN ? "RADIAN" : "DEGREE");
        return 1;
    }

    if (strcmp(line, "vars") == 0) {
        printf("\n=== Variables ===\n");
        for (size_t i = 0; i < repl->vars.count; i++) {
            char *s = value_to_string(&repl->vars.symbols[i].value);
            printf("  %s = %s\n", repl->vars.symbols[i].name, s ? s : "?");
            free(s);
        }
        printf("\n");
        return 1;
    }

    return 0;
}

int execute_repl_cycle(ReplState *repl, const char *raw_line) {
    char *line = strdup(raw_line);
    if (!line) return 0;
    trim_whitespace(line);

    if (handle_repl_command(repl, line)) {
        free(line);
        return 1;
    }

    /* expression evaluation comes in later commits */
    free(line);
    return 1;
}

int run_repl(void) {
    ReplState repl;
    repl_state_init(&repl);

    printf("=== Arbitrary-Precision Calculator ===\n");
    printf("Type 'help' for commands, 'exit' or Ctrl+D to quit.\n\n");

    while (repl.running) {
        display_prompt();
        char *line = read_input_line(stdin);
        if (!line) {   /* Ctrl+D */
            printf("\nExiting.\n");
            break;
        }
        execute_repl_cycle(&repl, line);
        free(line);
    }

    repl_state_free(&repl);
    return 0;
}