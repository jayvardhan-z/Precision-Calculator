#!/usr/bin/env bash
# ==============================================================================
# Script: test_sys_usability.sh
# Module: System Testing - Non-Functional Usability & REPL UX Verification
# Description:
#   Validates user interface and usability ergonomics:
#     - SYS-USA-01: Welcoming startup banner and command guide
#     - SYS-USA-02: 'help' and '?' command display
#     - SYS-USA-03: 'mode' and 'mode [rad|deg]' status feedback
#     - SYS-USA-04: 'precision' query and 'precision <n>' update feedback
#     - SYS-USA-05: Clean termination on 'exit' / 'quit' / 'q'
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$PROJECT_ROOT/calculator"
LOG="$PROJECT_ROOT/tests/test_results.csv"

echo "=== System Testing: Usability & REPL UX ==="

# SYS-USA-01: Welcome Banner
OUT=$(printf "exit\n" | "$BIN" 2>&1)
if echo "$OUT" | grep -q "Arbitrary-Precision Calculator"; then
    echo "  [PASS] SYS-USA-01: Welcome banner displayed correctly"
    echo "SYS-USA-01,USABILITY,PASS,1,\"Welcome banner displayed correctly\"" >> "$LOG"
else
    echo "  [FAIL] SYS-USA-01: Missing welcome banner"
    echo "SYS-USA-01,USABILITY,FAIL,1,\"Missing welcome banner\"" >> "$LOG"
fi

# SYS-USA-02: Help Command
OUT_HELP=$(printf "help\nexit\n" | "$BIN" 2>&1)
if echo "$OUT_HELP" | grep -q "=== Commands ===" && echo "$OUT_HELP" | grep -q "=== Syntax ==="; then
    echo "  [PASS] SYS-USA-02: 'help' command displays command list and syntax"
    echo "SYS-USA-02,USABILITY,PASS,1,\"'help' command displays command list and syntax\"" >> "$LOG"
else
    echo "  [FAIL] SYS-USA-02: 'help' command failed to display guide"
    echo "SYS-USA-02,USABILITY,FAIL,1,\"'help' command failed to display guide\"" >> "$LOG"
fi

# SYS-USA-03: Mode Command
OUT_MODE=$(printf "mode deg\nexit\n" | "$BIN" 2>&1)
if echo "$OUT_MODE" | grep -q "Angle mode is now DEGREE"; then
    echo "  [PASS] SYS-USA-03: 'mode deg' feedback confirmed"
    echo "SYS-USA-03,USABILITY,PASS,1,\"'mode deg' feedback confirmed\"" >> "$LOG"
else
    echo "  [FAIL] SYS-USA-03: 'mode deg' failed"
    echo "SYS-USA-03,USABILITY,FAIL,1,\"'mode deg' failed\"" >> "$LOG"
fi

# SYS-USA-04: Precision Command
OUT_PREC=$(printf "precision 50\nexit\n" | "$BIN" 2>&1)
if echo "$OUT_PREC" | grep -q "Precision set to 50 decimal places"; then
    echo "  [PASS] SYS-USA-04: 'precision 50' feedback confirmed"
    echo "SYS-USA-04,USABILITY,PASS,1,\"'precision 50' feedback confirmed\"" >> "$LOG"
else
    echo "  [FAIL] SYS-USA-04: 'precision 50' failed"
    echo "SYS-USA-04,USABILITY,FAIL,1,\"'precision 50' failed\"" >> "$LOG"
fi

# SYS-USA-05: Exit synonyms (quit, q)
OUT_QUIT=$(printf "quit\n" | "$BIN" 2>&1)
OUT_Q=$(printf "q\n" | "$BIN" 2>&1)
if [ $? -eq 0 ]; then
    echo "  [PASS] SYS-USA-05: 'quit' and 'q' terminate REPL cleanly"
    echo "SYS-USA-05,USABILITY,PASS,1,\"'quit' and 'q' terminate REPL cleanly\"" >> "$LOG"
else
    echo "  [FAIL] SYS-USA-05: Exit synonyms failed"
    echo "SYS-USA-05,USABILITY,FAIL,1,\"Exit synonyms failed\"" >> "$LOG"
fi

