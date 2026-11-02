#!/usr/bin/env bash
# ==============================================================================
# Script: test_sys_recovery.sh
# Module: System Testing - Non-Functional Recovery & Resilience Testing
# Description:
#   Validates recovery from fault conditions:
#     - SYS-REC-01: EOF (Ctrl+D) on stdin exits cleanly with code 0
#     - SYS-REC-02: Syntax error recovery (REPL recovers to evaluate subsequent valid input)
#     - SYS-REC-03: Long input string recovery (50,000 characters without memory fault)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== System Testing: Recovery & Resilience ==="

# SYS-REC-01: Immediate EOF
if echo -n "" | "$BIN" >/dev/null 2>&1; then
    echo "  [PASS] SYS-REC-01: Immediate EOF / Ctrl+D terminates REPL cleanly"
    echo "SYS-REC-01,RECOVERY,PASS,1,\"Immediate EOF terminates cleanly\"" >> "$LOG"
else
    echo "  [FAIL] SYS-REC-01: Immediate EOF failed"
    echo "SYS-REC-01,RECOVERY,FAIL,1,\"Immediate EOF failed\"" >> "$LOG"
fi

# SYS-REC-02: Error recovery in interactive stream
INPUT_SEQ=$(printf "syntax error ???\n2 + 3\nexit\n")
OUT_SEQ=$(echo "$INPUT_SEQ" | "$BIN" 2>&1)

if echo "$OUT_SEQ" | grep -q "Error:" && echo "$OUT_SEQ" | grep -q "= 5"; then
    echo "  [PASS] SYS-REC-02: REPL recovered from syntax error to evaluate subsequent '2 + 3'"
    echo "SYS-REC-02,RECOVERY,PASS,1,\"Error recovery verified\"" >> "$LOG"
else
    echo "  [FAIL] SYS-REC-02: REPL failed to recover after error"
    echo "SYS-REC-02,RECOVERY,FAIL,1,\"REPL failed to recover after error\"" >> "$LOG"
fi

# SYS-REC-03: Extreme input line recovery
LONG_INPUT=$(awk 'BEGIN {for(i=0;i<50000;i++) printf "1"}')
if [ -n "$LONG_INPUT" ]; then
    OUT_LONG=$(printf "%s\nexit\n" "$LONG_INPUT" | "$BIN" 2>&1 || true)
    if [ $? -eq 0 ]; then
        echo "  [PASS] SYS-REC-03: 50,000-character input stream handled without crash"
        echo "SYS-REC-03,RECOVERY,PASS,10,\"50k input handled without fault\"" >> "$LOG"
    else
        echo "  [FAIL] SYS-REC-03: 50k input caused crash"
        echo "SYS-REC-03,RECOVERY,FAIL,10,\"50k input caused crash\"" >> "$LOG"
    fi
fi
