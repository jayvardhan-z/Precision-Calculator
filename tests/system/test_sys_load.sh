#!/usr/bin/env bash
# ==============================================================================
# Script: test_sys_load.sh
# Module: System Testing - Continuous Load Testing
# Description:
#   Pipes 2,000 sequential expressions into the calculator binary via stdin.
#   Verifies system stability, pipeline throughput, and clean exit without crash.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

START_TIME=$(date +%s)

echo "=== System Testing: Load Testing (2,000 operations) ==="

TMP_INPUT=$(mktemp)
for i in $(seq 1 2000); do
    echo "$i + $i" >> "$TMP_INPUT"
done
echo "exit" >> "$TMP_INPUT"

if "$BIN" < "$TMP_INPUT" > /dev/null 2>&1; then
    END_TIME=$(date +%s)
    DIFF_S=$(( END_TIME - START_TIME ))
    DIFF_MS=$(( DIFF_S * 1000 ))
    echo "  [PASS] SYS-LOAD-01: 2000 expressions processed cleanly in ~${DIFF_MS} ms"
    echo "SYS-LOAD-01,SYSTEM,PASS,${DIFF_MS},\"Processed 2000 sequential expressions via REPL stdin\"" >> "$LOG"
else
    echo "  [FAIL] SYS-LOAD-01: Calculator crashed under continuous load"
    echo "SYS-LOAD-01,SYSTEM,FAIL,0,\"Calculator crashed under continuous load\"" >> "$LOG"
    rm -f "$TMP_INPUT"
    exit 1
fi

rm -f "$TMP_INPUT"
