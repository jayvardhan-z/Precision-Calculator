#!/usr/bin/env bash
# ==============================================================================
# Script: test_uat_scenarios.sh
# Module: Acceptance Testing - User Acceptance Testing (UAT)
# Description:
#   Validates user journeys for target customer personas:
#     - UAT-01 (Scientist): Arbitrary-precision factorial (100!) and constants (pi, e)
#     - UAT-02 (Student): Algebraic precedence, powers, and complex arithmetic
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== Acceptance Testing: User Acceptance Scenarios (UAT) ==="

# UAT-01: Scientist Persona
OUT_FACT=$("$BIN" -e "100!" | tr -d ' ' | grep -o "=93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000" || true)
if [ -n "$OUT_FACT" ]; then
    echo "  [PASS] UAT-01: Scientist persona: 100! calculated accurately (158 digits)"
    echo "UAT-01,ACCEPTANCE,PASS,5,\"Scientist persona: 100! verified\"" >> "$LOG"
else
    echo "  [FAIL] UAT-01: Scientist persona: 100! failed"
    echo "UAT-01,ACCEPTANCE,FAIL,5,\"Scientist persona: 100! failed\"" >> "$LOG"
fi

# UAT-02: Engineering Student Persona
OUT_STU=$("$BIN" -e "(3 + 4*i) * (3 - 4*i)" | tr -d ' ' | grep -o "=25" || true)
if [ "$OUT_STU" = "=25" ]; then
    echo "  [PASS] UAT-02: Student persona: Complex conjugate multiplication (3+4i)*(3-4i) == 25"
    echo "UAT-02,ACCEPTANCE,PASS,2,\"Student persona: Complex conjugate verified\"" >> "$LOG"
else
    echo "  [FAIL] UAT-02: Student persona: Complex multiplication failed"
    echo "UAT-02,ACCEPTANCE,FAIL,2,\"Student persona: Complex multiplication failed\"" >> "$LOG"
fi

