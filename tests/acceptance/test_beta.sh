#!/usr/bin/env bash
# ==============================================================================
# Script: test_beta.sh
# Module: Acceptance Testing - Beta Testing (Simulated User Community Trial)
# Description:
#   Replays a realistic interactive user session from beta_session.txt through
#   the calculator REPL, validating the overall user experience and outputs.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
SESSION="$PROJECT_ROOT/tests/acceptance/beta_session.txt"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== Acceptance Testing: Beta Testing User Session Replay ==="

RAW_OUTPUT=$("$BIN" < "$SESSION" 2>&1 || true)

BETA_FAIL=0

# Verify help was shown
if echo "$RAW_OUTPUT" | grep -q "=== Commands ==="; then
    echo "  [PASS] BETA-01: Help menu displayed in interactive session"
else
    echo "  [FAIL] BETA-01: Help menu not displayed"
    BETA_FAIL=$((BETA_FAIL + 1))
fi

# Verify 1/7 with precision 10 -> 0.1428571428
if echo "$RAW_OUTPUT" | grep -q "0.1428571428"; then
    echo "  [PASS] BETA-02: 1 / 7 evaluated to 10 decimal places"
else
    echo "  [FAIL] BETA-02: 1 / 7 precision failed"
    BETA_FAIL=$((BETA_FAIL + 1))
fi

# Verify typo was reported with Error
if echo "$RAW_OUTPUT" | grep -q "Error: Incorrect expression"; then
    echo "  [PASS] BETA-03: User typo reported descriptive error"
else
    echo "  [FAIL] BETA-03: Typo not reported"
    BETA_FAIL=$((BETA_FAIL + 1))
fi

# Verify ans * 10 produced 50
if echo "$RAW_OUTPUT" | grep -q "= 50"; then
    echo "  [PASS] BETA-04: Variable 'ans' chained properly (5 * 10 = 50)"
else
    echo "  [FAIL] BETA-04: ans chaining failed"
    BETA_FAIL=$((BETA_FAIL + 1))
fi

# Verify sin(30) in degrees produced 0.5000000000
if echo "$RAW_OUTPUT" | grep -q "0.5000000000"; then
    echo "  [PASS] BETA-05: sin(30 deg) produced 0.5000000000"
else
    echo "  [FAIL] BETA-05: sin(30 deg) failed"
    BETA_FAIL=$((BETA_FAIL + 1))
fi

# Log results
if [ $BETA_FAIL -eq 0 ]; then
    echo "BETA-01,BETA,PASS,1,\"Beta trial session passed with zero regressions\"" >> "$LOG"
else
    echo "BETA-01,BETA,FAIL,1,\"Beta trial session encountered failures\"" >> "$LOG"
    exit 1
fi

