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

/* returns 1 if the line was a command, 0 if it should be evaluated */
static int handle_repl_command(ReplState *repl, const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line) return 1;   /* empty line, nothing to do */

    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
        repl->running = 0;
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