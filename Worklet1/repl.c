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

// prints the input with a ^ under the column where the error is
static void print_error_caret(const char *line, size_t pos, const char *msg) {
    printf("  %s\n  ", line);
    size_t col = (pos > 0) ? pos - 1 : 0;
    for (size_t i = 0; i < col; i++) putchar(' ');
    printf("^\n  %s\n\n", msg);
}

// if line starts with the command name, return a pointer to its argument
static const char *command_args(const char *line, const char *name) {
    size_t n = strlen(name);
    if (strncmp(line, name, n) != 0) return NULL;
    if (line[n] != '\0' && !isspace((unsigned char)line[n])) return NULL;
    const char *p = line + n;
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

// returns 1 if the line was a command, 0 if it should be evaluated
static int handle_repl_command(ReplState *repl, const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    if (!*line) return 1;   // empty line, nothing to do

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
                symtab_preload_constants(&repl->vars, p);   // recompute pi, e
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
            // no argument, so just toggle
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
            printf("  %s = %s\n", repl->vars.symbols[i].name, s);
            free(s);
        }
        printf("\n");
        return 1;
    }

    return 0;
}

// handles "name = expr". Returns the variable name (malloc'd) and
// points *expr at the text after the '='. Returns NULL if it isn't an assignment.
static char *parse_assignment(char *line, char **expr) {
    char *eq = strchr(line, '=');
    if (!eq) return NULL;

    // the name must start with a letter or underscore
    char *start = line;
    while (isspace((unsigned char)*start)) start++;
    if (!isalpha((unsigned char)*start) && *start != '_') return NULL;

    // read the whole name
    char *end = start;
    while (isalnum((unsigned char)*end) || *end == '_') end++;

    // only spaces are allowed between the name and '='
    char *after = end;
    while (isspace((unsigned char)*after)) after++;
    if (after != eq) return NULL;

    size_t len = (size_t)(end - start);
    char *name = malloc(len + 1);
    memcpy(name, start, len);
    name[len] = '\0';

    *expr = eq + 1;
    return name;
}

int execute_repl_cycle(ReplState *repl, const char *raw_line) {
    // make our own copy so we can trim it without touching the original
    char *line = strdup(raw_line);
    trim_whitespace(line);

    // commands like exit/help/mode are handled separately
    if (handle_repl_command(repl, line)) {
        free(line);
        return 1;
    }

    TokenList tokens;
    ParsedExpression pe;
    ParseError perr;
    Value result;
    size_t err_pos = 0;
    int ok = 0;

    token_list_init(&tokens);
    parsed_expression_init(&pe);
    value_init(&result, VAL_INT);
    memset(&perr, 0, sizeof(perr));

    // check if the line is an assignment like "x = 5 + 2"
    // if it is, expr points to the part after the '='
    char *expr = line;
    char *var_name = parse_assignment(line, &expr);
    if (var_name) trim_whitespace(expr);

    // step 1: check brackets and basic syntax
    if (!validate_expression(expr, &err_pos)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ParseError: invalid syntax at column %zu", err_pos);
        print_error_caret(expr, err_pos, msg);
    }
    // step 2: break the input into tokens
    else if (!lex_tokenize(expr, &tokens, &perr)) {
        print_error_caret(expr, perr.position, perr.message);
    }
    // step 3: build the expression tree from the tokens
    else if (!parse_expression_tree(&tokens, &pe)) {
        print_error_caret(expr, pe.error.position, pe.error.message);
    }
    // step 4: evaluate the tree
    else {
        // clear old error before evaluating
        repl->ctx.depth = 0;
        repl->ctx.error.code = ERR_NONE;
        repl->ctx.error.pos = 0;
        repl->ctx.error.message[0] = '\0';

        // time only the evaluation, not the parsing
        Timer timer;
        timer_start(&timer);
        int eval_ok = eval_node(pe.root, &repl->ctx, &result);
        timer_stop(&timer);

        if (!eval_ok) {
            char msg[512];
            snprintf(msg, sizeof(msg), "RuntimeError: %s (at col %zu)",
                     repl->ctx.error.message, repl->ctx.error.pos);
            print_error_caret(expr, repl->ctx.error.pos, msg);
        } else {
            // convert result to text and print it with the time taken
            char *res_str = value_to_string(&result);
            if (var_name)
                printf("  %s = %s  [%.4f ms]\n\n", var_name, res_str, timer_elapsed_ms(&timer));
            else
                printf("  = %s  [%.4f ms]\n\n", res_str, timer_elapsed_ms(&timer));
            free(res_str);

            // save the result as "ans" so the next line can use it
            symtab_set(&repl->vars, "ans", &result);
            // also save it under the variable name if there was one
            if (var_name) symtab_set(&repl->vars, var_name, &result);
            ok = 1;
        }
    }

    // free everything we allocated
    free(var_name);
    value_free(&result);
    parsed_expression_free(&pe);
    token_list_free(&tokens);
    free(line);
    return ok;
}

int run_repl(void) {
    ReplState repl;
    repl_state_init(&repl);

    printf("=== Arbitrary-Precision Calculator ===\n");
    printf("Type 'help' for commands, 'exit' or Ctrl+D to quit.\n\n");

    while (repl.running) {
        display_prompt();
        char *line = read_input_line(stdin);
        if (!line) {   // Ctrl+D
            printf("\nExiting.\n");
            break;
        }
        execute_repl_cycle(&repl, line);
        free(line);
    }

    repl_state_free(&repl);
    return 0;
}