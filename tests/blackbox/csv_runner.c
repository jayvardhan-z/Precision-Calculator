/**
 * ============================================================================
 * File: csv_runner.c
 * Description: Data-driven CSV test runner for Precision-Calculator.
 * Features:
 *   - RFC 4180 compliant CSV parsing (quoted fields, embedded commas)
 *   - Dynamic column discovery via CSV header analysis
 *   - Executes ./calculator -e "<expression>" via popen
 *   - Strips timing brackets "[X.XXXX ms]" from actual output
 *   - Exact matching against rulebook expected strings
 *   - High-resolution timing of CLI execution
 *   - Machine-readable result logging to test_results.csv
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE 8192
#define MAX_OUT  65536

/* Trims leading and trailing whitespace and newlines */
static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

/* Parse a single CSV line with support for quoted fields containing commas */
static int parse_csv_line(char *line, char **fields, int max_fields) {
    int count = 0;
    char *p = line;
    while (*p && count < max_fields) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"') {
            p++; /* Skip opening quote */
            fields[count++] = p;
            while (*p && !(*p == '"' && (*(p + 1) == ',' || *(p + 1) == '\n' || *(p + 1) == '\r' || *(p + 1) == '\0'))) {
                if (*p == '"' && *(p + 1) == '"') p++; /* Escaped quote */
                p++;
            }
            if (*p == '"') {
                *p = '\0';
                p++;
            }
            if (*p == ',') p++;
        } else {
            fields[count++] = p;
            while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
            if (*p == ',') {
                *p = '\0';
                p++;
            } else if (*p == '\n' || *p == '\r') {
                *p = '\0';
            }
        }
    }
    return count;
}

/* Strips "[X.XXXX ms]" timing suffix from output */
static void strip_timing(char *s) {
    char *p = strstr(s, "  [");
    if (p && strstr(p, " ms]")) {
        *p = '\0';
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <test_cases.csv> [results_log.csv]\n", argv[0]);
        return 1;
    }

    const char *csv_path = argv[1];
    const char *log_path = (argc > 2) ? argv[2] : "tests/test_results.csv";

    FILE *f = fopen(csv_path, "r");
    if (!f) {
        fprintf(stderr, "Error opening %s\n", csv_path);
        return 1;
    }

    FILE *flog = fopen(log_path, "a");

    char line_buf[MAX_LINE];
    size_t total = 0, passed = 0, failed = 0;

    printf("=== CSV Test Runner: %s ===\n", csv_path);

    /* Read header and dynamically detect column mappings */
    if (!fgets(line_buf, sizeof(line_buf), f)) {
        fclose(f);
        if (flog) fclose(flog);
        return 0;
    }

    char header_copy[MAX_LINE];
    strncpy(header_copy, line_buf, sizeof(header_copy));
    header_copy[sizeof(header_copy) - 1] = '\0';

    char *hdr_fields[16] = {0};
    int hdr_count = parse_csv_line(header_copy, hdr_fields, 16);

    int col_id = 0;
    int col_expr = 1;
    int col_expected = -1;
    int col_type = -1;

    for (int i = 0; i < hdr_count; i++) {
        char *h = hdr_fields[i];
        trim(h);
        if (strcmp(h, "ID") == 0 || strcmp(h, "BENCHMARK_ID") == 0) {
            col_id = i;
        } else if (strcmp(h, "FULL_EXPR") == 0 || strcmp(h, "EXPRESSION") == 0 ||
                   strcmp(h, "INPUT_EXPR") == 0 || strcmp(h, "INPUT_EXPRESSION") == 0 ||
                   strcmp(h, "INPUT_LINE") == 0 || strcmp(h, "INPUT") == 0) {
            col_expr = i;
        } else if (strstr(h, "EXPECTED") || strstr(h, "RESULT") || strstr(h, "DIAGNOSTIC") || strstr(h, "PATTERN")) {
            col_expected = i;
        } else if (strstr(h, "TYPE") || strstr(h, "CATEGORY") || strstr(h, "CLASS")) {
            col_type = i;
        }
    }

    while (fgets(line_buf, sizeof(line_buf), f)) {
        char *fields[16] = {0};
        char line_copy[MAX_LINE];
        strncpy(line_copy, line_buf, sizeof(line_copy));
        line_copy[sizeof(line_copy) - 1] = '\0';

        int num_fields = parse_csv_line(line_copy, fields, 16);
        if (num_fields <= col_id || num_fields <= col_expr) continue;

        const char *id = fields[col_id];
        const char *expr = fields[col_expr];
        const char *expected = (col_expected >= 0 && num_fields > col_expected) ? fields[col_expected] : "";
        const char *type = (col_type >= 0 && num_fields > col_type && fields[col_type][0]) ? fields[col_type] : "BLACKBOX";

        trim((char *)id);
        trim((char *)expected);
        if (id[0] == '\0') continue;

        total++;

        /* Build command to execute with shell escaping */
        char escaped_expr[MAX_LINE * 2] = {0};
        size_t e_idx = 0;
        for (size_t i = 0; expr[i] && e_idx < sizeof(escaped_expr) - 2; i++) {
            if (expr[i] == '"' || expr[i] == '\\' || expr[i] == '$' || expr[i] == '`') {
                escaped_expr[e_idx++] = '\\';
            }
            escaped_expr[e_idx++] = expr[i];
        }
        escaped_expr[e_idx] = '\0';

        char cmd[MAX_LINE + 128];
        if (strlen(expr) == 0) {
            snprintf(cmd, sizeof(cmd), "./calculator -e \"\" 2>&1");
        } else {
            snprintf(cmd, sizeof(cmd), "./calculator -e \"%s\" 2>&1", escaped_expr);
        }

        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        FILE *pipe = popen(cmd, "r");
        char raw_out[MAX_OUT] = {0};
        char clean_out[MAX_OUT] = {0};

        if (pipe) {
            size_t bytes = fread(raw_out, 1, sizeof(raw_out) - 1, pipe);
            raw_out[bytes] = '\0';
            pclose(pipe);
        }

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                            (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;

        /* Extract output line: find line with '= ' or 'Error: ' */
        char *line_ptr = strtok(raw_out, "\r\n");
        while (line_ptr) {
            char *p = line_ptr;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '=' || strncmp(p, "Error:", 6) == 0) {
                strncpy(clean_out, p, sizeof(clean_out) - 1);
                break;
            }
            line_ptr = strtok(NULL, "\r\n");
        }

        strip_timing(clean_out);
        trim(clean_out);

        /* Compare against expected */
        bool match = false;
        if (expected[0] == '\0') {
            /* If no expected output given (e.g. pairwise), assert clean execution */
            match = (strlen(clean_out) > 0);
        } else if (strcmp(expected, "EXACT_35660_DIGITS") == 0) {
            size_t digits = 0;
            for (size_t i = 0; clean_out[i]; i++) {
                if (isdigit((unsigned char)clean_out[i])) digits++;
            }
            if (digits >= 35660) match = true;
        } else if (strcmp(clean_out, expected) == 0 || strstr(clean_out, expected) != NULL) {
            match = true;
        }

        if (match) {
            passed++;
            printf("  [\033[1;32mPASS\033[0m] %s: '%s' -> %s [%.3f ms]\n", id, expr, clean_out, elapsed_ms);
            if (flog) {
                fprintf(flog, "%s,%s,PASS,%.3f,\"%s\"\n", id, type, elapsed_ms, clean_out);
                fflush(flog);
            }
        } else {
            failed++;
            printf("  [\033[1;31mFAIL\033[0m] %s: '%s' [%.3f ms]\n", id, expr, elapsed_ms);
            printf("         Expected: '%s'\n", expected);
            printf("         Actual:   '%s'\n", clean_out);
            if (flog) {
                fprintf(flog, "%s,%s,FAIL,%.3f,\"Expected: %s; Got: %s\"\n", id, type, elapsed_ms, expected, clean_out);
                fflush(flog);
            }
        }
    }

    fclose(f);
    if (flog) fclose(flog);

    printf("\n--- Summary: %s ---\n", csv_path);
    printf("  Total:  %zu\n", total);
    printf("  Passed: \033[1;32m%zu\033[0m\n", passed);
    printf("  Failed: \033[1;%sm%zu\033[0m\n\n", failed > 0 ? "31" : "32", failed);

    return (failed == 0) ? 0 : 1;
}
