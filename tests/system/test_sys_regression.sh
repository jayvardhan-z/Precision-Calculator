#!/usr/bin/env bash
# ==============================================================================
# Script: test_sys_regression.sh
# Module: System Testing - Non-Functional Regression Oracle Testing
# Description:
#   Feeds golden expression test vectors from tests/data/golden_results.txt
#   into ./calculator -e and asserts byte-exact match with historical baseline.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
DATA="$PROJECT_ROOT/tests/data/golden_results.txt"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== System Testing: Regression Oracle vs Baseline ==="

FAILURES=0
TOTAL=0

while IFS= read -r line || [ -n "$line" ]; do
    # Skip comments or empty lines
    [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue

    TOTAL=$((TOTAL + 1))
    # Format: EXPR: <expr> -> <expected>
    EXPR=$(echo "$line" | sed -E 's/^EXPR:[[:space:]]*//; s/[[:space:]]*->.*//')
    EXPECTED=$(echo "$line" | sed -E 's/^.*->[[:space:]]*//')

    # Run calculator
    RAW_OUT=$("$BIN" -e "$EXPR" 2>&1 || true)
    # Strip leading whitespace and timing suffix "[... ms]"
    ACTUAL=$(echo "$RAW_OUT" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*\[[0-9.]+ ms\][[:space:]]*$//')

    if [ "$ACTUAL" = "$EXPECTED" ]; then
        echo "  [PASS] REG-$TOTAL: '$EXPR' == '$EXPECTED'"
        echo "SYS-REG-$TOTAL,REGRESSION,PASS,1,\"Golden match for $EXPR\"" >> "$LOG"
    else
        echo "  [FAIL] REG-$TOTAL: '$EXPR'"
        echo "         Expected: '$EXPECTED'"
        echo "         Actual:   '$ACTUAL'"
        echo "SYS-REG-$TOTAL,REGRESSION,FAIL,1,\"Mismatch for $EXPR: expected '$EXPECTED' got '$ACTUAL'\"" >> "$LOG"
        FAILURES=$((FAILURES + 1))
    fi
done < "$DATA"

echo "--- Regression Summary: Total: $TOTAL, Failed: $FAILURES ---"
if [ "$FAILURES" -ne 0 ]; then
    exit 1
fi

