/**
 * ============================================================================
 * File: gen_pairwise.c
 * Description: Standalone C program generating pairwise combinatorial test vectors.
 * Constraint: Zero external libraries/tools. Pure ISO C11.
 * Combinations:
 *   Operator (+, -, *, /, %, ^) x Left Type x Right Type x Sign (+, -, 0) x Angle Mode x Precision
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *OPERATORS[] = { "+", "-", "*", "/", "%", "^" };
static const size_t NUM_OPS = sizeof(OPERATORS) / sizeof(OPERATORS[0]);

static const char *LEFT_VALUES[] = {
    "10",            /* INT positive */
    "-10",           /* INT negative */
    "0",             /* INT zero */
    "2.5",           /* DEC positive */
    "-2.5",          /* DEC negative */
    "(1 + 2*i)",     /* COMPLEX */
};
static const size_t NUM_LEFT = sizeof(LEFT_VALUES) / sizeof(LEFT_VALUES[0]);

static const char *RIGHT_VALUES[] = {
    "2",             /* INT positive non-zero */
    "-2",            /* INT negative non-zero */
    "0",             /* INT zero */
    "1.5",           /* DEC positive non-zero */
    "(2 - 1*i)"      /* COMPLEX */
};
static const size_t NUM_RIGHT = sizeof(RIGHT_VALUES) / sizeof(RIGHT_VALUES[0]);

static const char *PRECISIONS[] = { "0", "20", "200" };
static const size_t NUM_PREC = sizeof(PRECISIONS) / sizeof(PRECISIONS[0]);

static const char *MODES[] = { "rad", "deg" };
static const size_t NUM_MODES = sizeof(MODES) / sizeof(MODES[0]);

int main(int argc, char *argv[]) {
    const char *out_path = "tests/data/test_cases_pairwise.csv";
    if (argc > 1) {
        out_path = argv[1];
    }

    FILE *f = fopen(out_path, "w");
    chmod(out_path, 0644);
    if (!f) {
        fprintf(stderr, "Error opening %s for writing\n", out_path);
        return 1;
    }

    fprintf(f, "ID,OPERATOR,LEFT_EXPR,RIGHT_EXPR,PRECISION,MODE,FULL_EXPR\n");

    /* Generate a comprehensive pairwise-covering subset */
    size_t id = 1;
    for (size_t op_idx = 0; op_idx < NUM_OPS; op_idx++) {
        for (size_t l_idx = 0; l_idx < NUM_LEFT; l_idx++) {
            /* Map right, precision, and mode using cyclic strides to cover pairs */
            size_t r_idx = (op_idx + l_idx) % NUM_RIGHT;
            size_t p_idx = (op_idx + r_idx) % NUM_PREC;
            size_t m_idx = (l_idx + r_idx) % NUM_MODES;

            const char *op = OPERATORS[op_idx];
            const char *l = LEFT_VALUES[l_idx];
            const char *r = RIGHT_VALUES[r_idx];
            const char *p = PRECISIONS[p_idx];
            const char *m = MODES[m_idx];

            fprintf(f, "PAIR-%03zu,%s,\"%s\",\"%s\",%s,%s,\"%s %s %s\"\n",
                    id++, op, l, r, p, m, l, op, r);
        }
    }

    fclose(f);
    printf("Successfully generated %zu pairwise combinations in %s\n", id - 1, out_path);
    return 0;
}
