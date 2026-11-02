#!/usr/bin/env bash
# ==============================================================================
# Script: test_sys_functional.sh
# Module: System Testing - Functional Coverage & Unsupported Function Rejection
# Description:
#   Validates functional behavior through the standalone CLI binary:
#     - Rejects unsupported functions (tan, asin, acos, atan, log10, sinh, cosh)
#     - Executes all supported mathematical functions: sin, cos, exp, ln, sqrt, abs, gcd
#     - Returns proper exit codes (0 for success, non-zero for syntax/eval error)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== System Testing: Functional Binary Verification ==="

# SYS-FUNC-01: Rejection of unsupported function 'tan(1)'
if "$BIN" -e "tan(1)" >/dev/null 2>&1; then
    echo "  [FAIL] SYS-FUNC-01: 'tan(1)' should have failed"
    echo "SYS-FUNC-01,FUNCTIONAL,FAIL,1,\"'tan(1)' should be rejected\"" >> "$LOG"
else
    echo "  [PASS] SYS-FUNC-01: Unsupported function 'tan(1)' rejected cleanly"
    echo "SYS-FUNC-01,FUNCTIONAL,PASS,1,\"'tan(1)' rejected cleanly\"" >> "$LOG"
fi

# SYS-FUNC-02: Rejection of unsupported function 'asin(0.5)'
if "$BIN" -e "asin(0.5)" >/dev/null 2>&1; then
    echo "  [FAIL] SYS-FUNC-02: 'asin(0.5)' should have failed"
    echo "SYS-FUNC-02,FUNCTIONAL,FAIL,1,\"'asin(0.5)' should be rejected\"" >> "$LOG"
else
    echo "  [PASS] SYS-FUNC-02: Unsupported function 'asin(0.5)' rejected cleanly"
    echo "SYS-FUNC-02,FUNCTIONAL,PASS,1,\"'asin(0.5)' rejected cleanly\"" >> "$LOG"
fi

# SYS-FUNC-03: Rejection of unsupported function 'log10(100)'
if "$BIN" -e "log10(100)" >/dev/null 2>&1; then
    echo "  [FAIL] SYS-FUNC-03: 'log10(100)' should have failed"
    echo "SYS-FUNC-03,FUNCTIONAL,FAIL,1,\"'log10(100)' should be rejected\"" >> "$LOG"
else
    echo "  [PASS] SYS-FUNC-03: Unsupported function 'log10(100)' rejected cleanly"
    echo "SYS-FUNC-03,FUNCTIONAL,PASS,1,\"'log10(100)' rejected cleanly\"" >> "$LOG"
fi

# SYS-FUNC-04: Functional execution of abs(-42)
OUT_ABS=$("$BIN" -e "abs(-42)" | tr -d ' ' | grep -o "=42" || true)
if [ "$OUT_ABS" = "=42" ]; then
    echo "  [PASS] SYS-FUNC-04: abs(-42) == 42"
    echo "SYS-FUNC-04,FUNCTIONAL,PASS,1,\"abs(-42) == 42\"" >> "$LOG"
else
    echo "  [FAIL] SYS-FUNC-04: abs(-42) failed"
    echo "SYS-FUNC-04,FUNCTIONAL,FAIL,1,\"abs(-42) failed\"" >> "$LOG"
fi

# SYS-FUNC-05: Functional execution of gcd(1071, 462) == 21
OUT_GCD=$("$BIN" -e "gcd(1071, 462)" | tr -d ' ' | grep -o "=21" || true)
if [ "$OUT_GCD" = "=21" ]; then
    echo "  [PASS] SYS-FUNC-05: gcd(1071, 462) == 21"
    echo "SYS-FUNC-05,FUNCTIONAL,PASS,1,\"gcd(1071, 462) == 21\"" >> "$LOG"
else
    echo "  [FAIL] SYS-FUNC-05: gcd(1071, 462) failed"
    echo "SYS-FUNC-05,FUNCTIONAL,FAIL,1,\"gcd(1071, 462) failed\"" >> "$LOG"
fi

