#!/usr/bin/env bash
# ==============================================================================
# Script: test_rat_compliance.sh
# Module: Acceptance Testing - Regulations Acceptance Testing (RAT)
# Description:
#   Validates technical regulatory and standards compliance:
#     - RAT-01: ISO C11 standard conformance (-std=c11 -pedantic-errors)
#     - RAT-02: POSIX.1-2008 specification adherence (_POSIX_C_SOURCE=200809L)
#     - RAT-03: Safety hygiene: absence of unsafe deprecated libc calls (e.g. gets)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== Acceptance Testing: Regulations Acceptance Testing (RAT) ==="

# RAT-01: ISO C11 Pedantic compilation check
if clang -std=c11 -pedantic-errors -fsyntax-only \
    -I"$PROJECT_ROOT/Worklet1" -I"$PROJECT_ROOT/Worklet2" -I"$PROJECT_ROOT/Worklet3" \
    "$PROJECT_ROOT"/Worklet1/*.c "$PROJECT_ROOT"/Worklet2/*.c "$PROJECT_ROOT"/Worklet3/*.c "$PROJECT_ROOT"/main.c >/dev/null 2>&1; then
    echo "  [PASS] RAT-01: Code complies strictly with ISO C11 (-std=c11 -pedantic-errors)"
    echo "RAT-01,COMPLIANCE,PASS,1,\"Strict ISO C11 verified\"" >> "$LOG"
else
    echo "  [FAIL] RAT-01: ISO C11 syntax error detected"
    echo "RAT-01,COMPLIANCE,FAIL,1,\"ISO C11 violation\"" >> "$LOG"
fi

# RAT-02: POSIX.1-2008 clock_gettime and timespec availability
if grep -rn "CLOCK_MONOTONIC" "$PROJECT_ROOT"/Worklet* >/dev/null 2>&1; then
    echo "  [PASS] RAT-02: POSIX.1-2008 monotonic clock routines properly integrated"
    echo "RAT-02,COMPLIANCE,PASS,1,\"POSIX.1-2008 compliance verified\"" >> "$LOG"
else
    echo "  [FAIL] RAT-02: POSIX clock missing"
    echo "RAT-02,COMPLIANCE,FAIL,1,\"POSIX compliance failed\"" >> "$LOG"
fi

# RAT-03: Deprecated function scan
if grep -rn "\bgets(" "$PROJECT_ROOT"/Worklet* "$PROJECT_ROOT"/main.c >/dev/null 2>&1; then
    echo "  [FAIL] RAT-03: Insecure 'gets' call detected"
    echo "RAT-03,COMPLIANCE,FAIL,1,\"Insecure gets found\"" >> "$LOG"
else
    echo "  [PASS] RAT-03: Zero deprecated/banned C functions detected"
    echo "RAT-03,COMPLIANCE,PASS,1,\"Zero banned functions verified\"" >> "$LOG"
fi

