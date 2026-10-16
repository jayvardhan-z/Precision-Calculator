#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// prints a simple error message
static void print_error(const char *msg) {
    printf("  Error: %s\n\n", msg);
}

// returns 1 if the line was a command, 0 if it should be evaluated
static int handle_repl_command(ReplState *repl, const char *line) {
    // empty line, nothing to do
    if (line[0] == '\0') return 1;

    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
        repl->running = 0;
        return 1;
    }

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

    // "precision" alone shows the current value
    if (strcmp(line, "precision") == 0) {
        printf("  [Current precision: %d]\n\n", repl->ctx.precision);
        return 1;
    }

    // "precision 30" sets a new value
    if (strncmp(line, "precision ", 10) == 0) {
        int p = atoi(line + 10);
        if (p >= 0 && p <= 2000) {
            repl->ctx.precision = p;
            symtab_preload_constants(&repl->vars, p);   // recompute pi, e
            printf("  [Precision set to %d decimal places]\n\n", p);
        } else {
            printf("  [Error: precision must be between 0 and 2000]\n\n");
        }
        return 1;
    }

    // "mode" alone toggles, "mode deg" / "mode rad" sets it
    if (strcmp(line, "mode") == 0 || strncmp(line, "mode ", 5) == 0) {
        const char *arg = line + 4;
        while (*arg == ' ') arg++;

        if (strcmp(arg, "deg") == 0 || strcmp(arg, "degree") == 0) {
            repl->ctx.angle_mode = ANGLE_DEGREE;
        } else if (strcmp(arg, "rad") == 0 || strcmp(arg, "radian") == 0) {
            repl->ctx.angle_mode = ANGLE_RADIAN;
        } else {
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

// checks for "name = expr". Returns the variable name (malloc'd),
// or NULL if the line is not an assignment.
static char *parse_assignment(char *line) {
    char *eq = strchr(line, '=');
    if (!eq) return NULL;

    // the name must start with a letter or underscore
    char *start = line;
    while (*start == ' ') start++;
    if (!((*start >= 'a' && *start <= 'z') ||
          (*start >= 'A' && *start <= 'Z') ||
          *start == '_')) return NULL;

    // read the whole name (letters, digits, underscore)
    char *end = start;
    while ((*end >= 'a' && *end <= 'z') ||
           (*end >= 'A' && *end <= 'Z') ||
           (*end >= '0' && *end <= '9') ||
           *end == '_') end++;

    // only spaces are allowed between the name and '='
    char *after = end;
    while (*after == ' ') after++;
    if (after != eq) return NULL;

    // copy the name
    int len = end - start;
    char *name = malloc(len + 1);
    memcpy(name, start, len);
    name[len] = '\0';
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

    // check if the line is an assignment like "x = 5 + 2"
    // if it is, expr is the part after the '='
    char *expr = line;
    char *var_name = parse_assignment(line);
    if (var_name) {
        expr = strchr(line, '=') + 1;
        trim_whitespace(expr);
    }

    // step 1: check brackets and basic syntax
    if (!validate_expression(expr, &err_pos)) {
        print_error("Incorrect expression");
    }
    // step 2: break the input into tokens
    else if (!lex_tokenize(expr, &tokens, &perr)) {
        print_error("Incorrect expression");
    }
    // step 3: build the expression tree from the tokens
    else if (!parse_expression_tree(&tokens, &pe)) {
        print_error("Incorrect expression");
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
            // show the reason from the evaluator, e.g. division by zero
            print_error(repl->ctx.error.message);
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