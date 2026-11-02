#!/usr/bin/env bash
# ============================================================================
# File: tests/blackbox/runner.sh
# Description: Wrapper that compiles and executes the C-based CSV test runner.
# Ensures 100% standard POSIX compliance and robust quoted CSV parsing.
# ============================================================================

set -euo pipefail

CSV_FILE="${1:-tests/data/test_cases_rulebook.csv}"
RESULTS_LOG="${2:-tests/test_results.csv}"

# Ensure csv_runner binary is compiled
if [[ ! -x "tests/blackbox/csv_runner" || "tests/blackbox/csv_runner.c" -nt "tests/blackbox/csv_runner" ]]; then
    cc -std=c11 -O2 tests/blackbox/csv_runner.c -o tests/blackbox/csv_runner
fi

exec ./tests/blackbox/csv_runner "$CSV_FILE" "$RESULTS_LOG"
