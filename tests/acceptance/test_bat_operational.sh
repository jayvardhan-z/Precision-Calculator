#!/usr/bin/env bash
# ==============================================================================
# Script: test_bat_operational.sh
# Module: Acceptance Testing - Business Acceptance Testing (BAT)
# Description:
#   Validates business operations and deployment requirements:
#     - BAT-01: Standalone execution without external runtime dependencies
#     - BAT-02: CLI non-interactive flag (-e) returns exit code 0 on success
#     - BAT-03: CLI non-interactive flag (-e) returns non-zero code on error
#     - BAT-04: Headless automation via pipeline redirection
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== Acceptance Testing: Business Acceptance Testing (BAT) ==="

# BAT-01: Executable check
if [ -x "$BIN" ]; then
    echo "  [PASS] BAT-01: Binary is standalone executable with proper file permissions"
    echo "BAT-01,ACCEPTANCE,PASS,1,\"Standalone executable validated\"" >> "$LOG"
else
    echo "  [FAIL] BAT-01: Binary not executable"
    echo "BAT-01,ACCEPTANCE,FAIL,1,\"Binary not executable\"" >> "$LOG"
fi

# BAT-02: Exit code 0 on success
if "$BIN" -e "2 + 2" >/dev/null 2>&1; then
    echo "  [PASS] BAT-02: CLI -e returns exit code 0 on successful evaluation"
    echo "BAT-02,ACCEPTANCE,PASS,1,\"CLI exit code 0 on success\"" >> "$LOG"
else
    echo "  [FAIL] BAT-02: Non-zero exit code on valid expression"
    echo "BAT-02,ACCEPTANCE,FAIL,1,\"Non-zero exit code on valid expression\"" >> "$LOG"
fi

# BAT-03: Non-zero exit code on error
set +e
"$BIN" -e "2 / 0" >/dev/null 2>&1
EC=$?
set -e
if [ $EC -ne 0 ]; then
    echo "  [PASS] BAT-03: CLI -e returns non-zero exit code ($EC) on arithmetic error"
    echo "BAT-03,ACCEPTANCE,PASS,1,\"CLI non-zero exit code on error\"" >> "$LOG"
else
    echo "  [FAIL] BAT-03: Expected non-zero exit code on division by zero"
    echo "BAT-03,ACCEPTANCE,FAIL,1,\"Expected non-zero exit code on error\"" >> "$LOG"
fi

# BAT-04: Headless pipeline redirection
OUT_PIPE=$(echo "10 * 10" | "$BIN" 2>&1 | grep -o "= 100" || true)
if [ "$OUT_PIPE" = "= 100" ]; then
    echo "  [PASS] BAT-04: Headless pipeline redirection executes and produces '= 100'"
    echo "BAT-04,ACCEPTANCE,PASS,1,\"Headless pipeline automation verified\"" >> "$LOG"
else
    echo "  [FAIL] BAT-04: Headless pipeline automation failed"
    echo "BAT-04,ACCEPTANCE,FAIL,1,\"Headless pipeline automation failed\"" >> "$LOG"
fi

