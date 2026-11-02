#!/usr/bin/env bash
# ==============================================================================
# Script: test_alpha.sh
# Module: Acceptance Testing - Alpha Testing (In-House Internal Release)
# Description:
#   Executes the internal Alpha test cycle simulating developer and internal
#   QA acceptance across the end-to-end binary build.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== Acceptance Testing: Alpha In-House Release Validation ==="

ALPHA_FAIL=0

# Alpha 1: Basic Arithmetic Integrity
OUT_A1=$("$BIN" -e "(100 + 200) * 3" | tr -d ' ' | grep -o "=900" || true)
if [ "$OUT_A1" = "=900" ]; then
    echo "  [PASS] ALPHA-01: Arithmetic pipeline verified"
    echo "ALPHA-01,ALPHA,PASS,1,\"Arithmetic verified\"" >> "$LOG"
else
    echo "  [FAIL] ALPHA-01: Arithmetic failed"
    echo "ALPHA-01,ALPHA,FAIL,1,\"Arithmetic failed\"" >> "$LOG"
    ALPHA_FAIL=$((ALPHA_FAIL + 1))
fi

# Alpha 2: High Precision Constants
OUT_A2=$("$BIN" -e "pi" | grep -o "3.14159265358979323846" || true)
if [ -n "$OUT_A2" ]; then
    echo "  [PASS] ALPHA-02: Constant pi high precision verified"
    echo "ALPHA-02,ALPHA,PASS,1,\"Constant pi verified\"" >> "$LOG"
else
    echo "  [FAIL] ALPHA-02: Constant pi failed"
    echo "ALPHA-02,ALPHA,FAIL,1,\"Constant pi failed\"" >> "$LOG"
    ALPHA_FAIL=$((ALPHA_FAIL + 1))
fi

# Alpha 3: Error Message Standard Format
OUT_A3=$("$BIN" -e "1 / 0" 2>&1 || true)
if echo "$OUT_A3" | grep -q "Error: Division by zero"; then
    echo "  [PASS] ALPHA-03: Error messaging standard format confirmed"
    echo "ALPHA-03,ALPHA,PASS,1,\"Error format confirmed\"" >> "$LOG"
else
    echo "  [FAIL] ALPHA-03: Unexpected error string: $OUT_A3"
    echo "ALPHA-03,ALPHA,FAIL,1,\"Unexpected error string\"" >> "$LOG"
    ALPHA_FAIL=$((ALPHA_FAIL + 1))
fi

if [ $ALPHA_FAIL -ne 0 ]; then
    exit 1
fi

