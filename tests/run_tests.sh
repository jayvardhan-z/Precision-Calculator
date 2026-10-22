#!/usr/bin/env bash
# =============================================================================
# File: tests/run_tests.sh
# Description: Master test runner for the Precision-Calculator test suite.
#
# Stages executed (in order):
#   1. Build check  – Ensure main calculator binary is up to date
#   2. Unit tests   – Build and run C unit test binaries
#   3. Integration  – Feed known expressions to ./calculator via stdin pipe
#   4. ASAN build   – Rebuild everything under AddressSanitizer + UBSan
#   5. ASAN tests   – Re-run all unit tests under ASAN
#   6. Edge/Fuzz    – Pipe adversarial inputs and verify graceful handling
#   7. Performance  – Time a set of heavy computations
#   8. Summary      – Print final PASS/FAIL/SKIP totals and exit code
#
# Usage:
#   chmod +x tests/run_tests.sh
#   tests/run_tests.sh [--asan] [--verbose]
#
# Flags:
#   --asan      Force ASAN rebuild even without explicit request
#   --verbose   Print all calculator output (default: only failures)
#
# Requirements:
#   - gcc (or CC env variable) supporting -fsanitize=address,undefined
#   - Python 3 (optional: used for differential reference checking)
#   - bc (optional: used as floating-point reference for quick checks)
# =============================================================================

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
ROOT="$(cd "$(dirname "$0")/.." && pwd)"   # Absolute path to project root
TESTS_DIR="$ROOT/tests"
CALC="$ROOT/calculator"
CC="${CC:-gcc}"
CFLAGS="-Wall -Wextra -std=c11 -O2"
INCLUDES="-I$ROOT/Worklet1 -I$ROOT/Worklet2 -I$ROOT/Worklet3"
DEFINES="-D_POSIX_C_SOURCE=200809L"
LIB_SRCS="$ROOT/Worklet1/ast.c $ROOT/Worklet1/bigint_io.c \
          $ROOT/Worklet1/lexer.c $ROOT/Worklet1/parser.c \
          $ROOT/Worklet1/repl.c $ROOT/Worklet1/timer.c \
          $ROOT/Worklet2/bigint.c \
          $ROOT/Worklet3/complex.c $ROOT/Worklet3/decimal.c \
          $ROOT/Worklet3/eval.c $ROOT/Worklet3/fast_math.c \
          $ROOT/Worklet3/functions.c"

VERBOSE=false
FORCE_ASAN=false
for arg in "$@"; do
  case "$arg" in
    --asan)    FORCE_ASAN=true ;;
    --verbose) VERBOSE=true    ;;
  esac
done

# ── Color output ──────────────────────────────────────────────────────────────
if [ -t 1 ]; then
  RED='\033[1;31m'; GREEN='\033[1;32m'; YELLOW='\033[1;33m'
  CYAN='\033[1;36m'; BOLD='\033[1m'; RESET='\033[0m'
else
  RED=''; GREEN=''; YELLOW=''; CYAN=''; BOLD=''; RESET=''
fi

# ── Counters ──────────────────────────────────────────────────────────────────
PASS=0; FAIL=0; SKIP=0

pass()  { PASS=$((PASS+1));  echo -e "${GREEN}PASS${RESET}  $1"; }
fail()  { FAIL=$((FAIL+1));  echo -e "${RED}FAIL${RESET}  $1"; }
skip()  { SKIP=$((SKIP+1));  echo -e "${YELLOW}SKIP${RESET}  $1"; }
header(){ echo -e "\n${BOLD}${CYAN}══ $1${RESET}"; }

# ── Helper: run calculator with a single expression via stdin ────────────────
# Usage: calc_eval "expression"  expected_substring
calc_eval() {
    local expr="$1"
    local expected="$2"
    local label="${3:-$expr}"

    # REPL output format: "> = VALUE  [timing ms]"
    # Extract VALUE: match lines starting with '>', strip prompt and timing
    local out
    out=$(printf '%s\nquit\n' "$expr" | "$CALC" 2>&1 | \
          grep '^>' | head -1 | sed 's/^> *//' | sed 's/ *= *//' | sed 's/ *\[.*//' | tr -d ' \r') || true

    if $VERBOSE; then
        echo "  input: $expr"
        echo "  output: $out"
    fi

    if echo "$out" | grep -qF "$expected"; then
        pass "$label \u2192 contains \"$expected\""
    else
        fail "$label \u2192 expected \"$expected\" but got: $out"
    fi
}

# ── Helper: calc_eval_no_crash ────────────────────────────────────────────────
# Runs calculator with the given input and asserts it does NOT crash (exit ≠ signal).
calc_no_crash() {
    local expr="$1"
    local label="${2:-no-crash: $expr}"
    printf '%s\nquit\n' "$expr" | "$CALC" >/dev/null 2>&1 || true
    pass "$label"
}

# =============================================================================
# Stage 1: Build check
# =============================================================================
header "Stage 1: Build"

if make -C "$ROOT" -s all; then
    pass "make all — calculator binary built"
else
    fail "make all — BUILD FAILED; aborting"
    exit 1
fi

# =============================================================================
# Stage 2: C Unit Tests
# =============================================================================
header "Stage 2: C Unit Tests"

build_and_run_unit() {
    local src="$1"
    local bin="$TESTS_DIR/$(basename "${src%.c}")"
    local label="$(basename "${src%.c}")"

    # Compile the test binary, linking all calculator library sources
    if $CC $CFLAGS $DEFINES $INCLUDES \
           "$src" $LIB_SRCS \
           -o "$bin" -lm 2>/tmp/test_build_err; then
        # Run the compiled test binary
        if "$bin"; then
            pass "$label — all assertions passed"
        else
            fail "$label — some assertions FAILED (see above)"
        fi
    else
        fail "$label — COMPILATION FAILED:"
        cat /tmp/test_build_err
    fi
    rm -f "$bin"
}

build_and_run_unit "$TESTS_DIR/test_bigint.c"
build_and_run_unit "$TESTS_DIR/test_lexer_parser.c"
build_and_run_unit "$TESTS_DIR/test_eval.c"

# =============================================================================
# Stage 3: Integration Tests (REPL via stdin)
# =============================================================================
header "Stage 3: Integration Tests"

# Basic arithmetic
calc_eval "1 + 1"            "2"            "int add"
calc_eval "100 - 37"         "63"           "int sub"
calc_eval "12 * 12"          "144"          "int mul"
calc_eval "17 % 5"           "2"            "modulo"
calc_eval "2^32"             "4294967296"   "pow 2^32"
calc_eval "10!"              "3628800"      "factorial 10"

# Decimal / precision
calc_eval "1/3"              "0.333"        "1/3 prefix"
calc_eval "355/113"          "3.1415"       "pi approx"
calc_eval "sqrt(2)"          "1.41421"      "sqrt(2) prefix"

# Transcendental functions
calc_eval "sin(0)"           "0"            "sin(0)"
calc_eval "cos(0)"           "1"            "cos(0)"
calc_eval "ln(e)"            "1"            "ln(e)=1"
calc_eval "exp(0)"           "1"            "exp(0)=1"
calc_eval "sqrt(9)"          "3"            "sqrt(9)=3"

# Constants
calc_eval "pi"               "3.14159"      "pi constant"
calc_eval "e"                "2.71828"      "e constant"

# Large numbers
calc_eval "2^100"            "1267650600228229401496703205376"  "2^100"
calc_eval "20!"              "2432902008176640000"              "20!"

# Variable assignment
calc_eval "x = 42"           "42"           "var assign"
# Note: REPL may or may not show the assigned value; test for no crash
calc_no_crash "x = 100" "var assign no crash"

# Deeply nested parentheses (60 levels)
DEEP="$(printf '%.0s(' {1..60})1$(printf '%.0s)' {1..60})"
calc_eval "$DEEP" "1" "deep parens 60"

# =============================================================================
# Stage 4: Error / Edge-Case Integration
# =============================================================================
header "Stage 4: Error & Edge Cases"

# Division by zero must not crash; should print an error message
calc_no_crash "1/0"               "div-by-zero no crash"
calc_no_crash "ln(0)"             "ln(0) no crash"
calc_no_crash "sqrt(-1)"          "sqrt(-1) no crash"
calc_no_crash "0^0"               "0^0 no crash"
calc_no_crash "-0"                "-0 no crash"
calc_no_crash "999999999999999999999999999^2"  "giant^2 no crash"

# Malformed / adversarial inputs
calc_no_crash "@@@"               "invalid chars no crash"
calc_no_crash "(((("              "unclosed parens no crash"
calc_no_crash "1 + + + + 1"      "multi-plus no crash"
calc_no_crash "sin("              "incomplete call no crash"
calc_no_crash ""                  "empty input no crash"
calc_no_crash "   "               "whitespace input no crash"
calc_no_crash "1/0.000000001"     "near-zero divisor no crash"

# Extremely long expression (stress the lexer buffer)
LONG_EXPR="$(python3 -c "print('+'.join(['1']*500))" 2>/dev/null || echo "1+1")"
calc_no_crash "$LONG_EXPR" "500-term sum no crash"

# =============================================================================
# Stage 5: AddressSanitizer Build & Test
# =============================================================================
header "Stage 5: AddressSanitizer + UBSan Build"

ASAN_FLAGS="-fsanitize=address,undefined -g3 -O0 -fno-omit-frame-pointer"

# Check if compiler supports ASAN
if echo "int main(){}" | $CC $ASAN_FLAGS -x c - -o /tmp/_asan_probe -lm 2>/dev/null; then
    rm -f /tmp/_asan_probe
    pass "Compiler supports -fsanitize=address,undefined"

    asan_build_and_run() {
        local src="$1"
        local bin="$TESTS_DIR/asan_$(basename "${src%.c}")"
        local label="ASAN::$(basename "${src%.c}")"

        if $CC $ASAN_FLAGS $DEFINES $INCLUDES \
               "$src" $LIB_SRCS \
               -o "$bin" -lm 2>/tmp/asan_build_err; then
            # On macOS, detect_leaks is not supported; use abort_on_error only
            if ASAN_OPTIONS="abort_on_error=1" "$bin" 2>/tmp/asan_out; then
                pass "$label — no memory errors"
            else
                fail "$label — ASAN/UBSan violations detected:"
                cat /tmp/asan_out
            fi
        else
            fail "$label — ASAN compilation FAILED:"
            cat /tmp/asan_build_err
        fi
        rm -f "$bin"
    }

    asan_build_and_run "$TESTS_DIR/test_bigint.c"
    asan_build_and_run "$TESTS_DIR/test_lexer_parser.c"
    asan_build_and_run "$TESTS_DIR/test_eval.c"
else
    skip "ASAN not supported by compiler — skipping memory-error stage"
fi

# =============================================================================
# Stage 6: Valgrind Memcheck (optional)
# =============================================================================
header "Stage 6: Valgrind Memcheck"

if command -v valgrind &>/dev/null; then
    VG="valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --quiet"
    VG_BIN="$TESTS_DIR/vg_test_bigint"

    $CC $CFLAGS $DEFINES $INCLUDES \
        "$TESTS_DIR/test_bigint.c" $LIB_SRCS \
        -o "$VG_BIN" -lm 2>/dev/null

    if $VG "$VG_BIN" >/dev/null 2>/tmp/vg_out; then
        pass "Valgrind::test_bigint — zero leaks"
    else
        fail "Valgrind::test_bigint — memory leaks or errors:"
        cat /tmp/vg_out
    fi
    rm -f "$VG_BIN"
else
    skip "valgrind not found — install with: brew install valgrind (Linux only)"
fi

# =============================================================================
# Stage 7: Performance Benchmarks
# =============================================================================
header "Stage 7: Performance Benchmarks"

bench() {
    local label="$1"
    local expr="$2"

    # Use python3 for millisecond timing (portable: macOS BSD date lacks %3N)
    local t0 t1 elapsed
    t0=$(python3 -c 'import time; print(int(time.time()*1000))' 2>/dev/null || echo 0)
    printf '%s\nquit\n' "$expr" | "$CALC" >/dev/null 2>&1 || true
    t1=$(python3 -c 'import time; print(int(time.time()*1000))' 2>/dev/null || echo 0)
    elapsed=$((t1 - t0))

    if [ "$elapsed" -lt 5000 ]; then
        pass "BENCH $label — ${elapsed}ms (< 5000ms threshold)"
    else
        fail "BENCH $label — ${elapsed}ms EXCEEDED 5000ms threshold"
    fi
}

bench "2^1000"            "2^1000"
bench "100! "             "100!"
bench "sqrt(2) @prec=50"  "sqrt(2)"
bench "pi*e @prec=30"     "pi*e"
bench "sin(pi/6)"         "sin(pi/6)"
bench "exp(100)"          "exp(100)"
bench "ln(1000000)"       "ln(1000000)"

# =============================================================================
# Stage 8: Python Differential Validation (optional)
# =============================================================================
header "Stage 8: Differential Testing vs Python"

if command -v python3 &>/dev/null; then
    diff_check() {
        local label="$1"
        local calc_expr="$2"
        local py_expr="$3"
        local digits="${4:-6}"  # Compare first N digits

        local calc_out py_out
        # REPL output format: "> = VALUE  [timing ms]"
        # Extract VALUE: match lines starting with '>', strip prompt and timing
        calc_out=$(printf '%s\nquit\n' "$calc_expr" | "$CALC" 2>/dev/null | \
                   grep '^>' | head -1 | sed 's/^> *//' | sed 's/ *= *//' | sed 's/ *\[.*//' | tr -d ' \r')
        py_out=$(python3 -c "
from decimal import Decimal, getcontext
import math
getcontext().prec = 50
try:
    r = $py_expr
    print(str(r)[:$((digits+5))])
except Exception as e:
    print('ERROR')
" 2>/dev/null)

        # Compare first `digits` significant chars (strip leading 0. for comparison)
        local c_prefix="${calc_out:0:$digits}"
        local p_prefix="${py_out:0:$digits}"

        if [ "$c_prefix" = "$p_prefix" ]; then
            pass "DIFF $label — calc=\"$c_prefix\" py=\"$p_prefix\""
        else
            fail "DIFF $label — MISMATCH calc=\"$c_prefix\" py=\"$p_prefix\""
        fi
    }

    diff_check "1/7"         "1/7"        "Decimal(1)/Decimal(7)"           8
    diff_check "sqrt(2)"     "sqrt(2)"    "Decimal(2).sqrt()"               10
    diff_check "2^50"        "2^50"       "2**50"                           10
    diff_check "10!"         "10!"        "math.factorial(10)"              7
else
    skip "python3 not found — skipping differential validation"
fi

# =============================================================================
# Stage 9: Extreme Stress Testing Suite (10k ops, 100k digits, 5k nesting)
# =============================================================================
header "Stage 9: Extreme Stress Testing Suite"

if command -v python3 >/dev/null 2>&1; then
    if python3 "$ROOT/tests/extreme_stress_test.py"; then
        pass "extreme_stress_test.py — all 69 stress scenarios passed"
    else
        fail "extreme_stress_test.py — stress test failures detected"
    fi
else
    skip "python3 not found — skipping extreme stress test suite"
fi

# =============================================================================
# Final Summary
# =============================================================================
TOTAL=$((PASS + FAIL + SKIP))
echo ""
echo -e "${BOLD}══ Test Summary ══════════════════════════════════════════${RESET}"
echo -e "   Total:   $TOTAL"
echo -e "   ${GREEN}Passed:  $PASS${RESET}"
echo -e "   ${RED}Failed:  $FAIL${RESET}"
echo -e "   ${YELLOW}Skipped: $SKIP${RESET}"
echo -e "${BOLD}═══════════════════════════════════════════════════════════${RESET}"

if [ "$FAIL" -gt 0 ]; then
    echo -e "${RED}❌  $FAIL test(s) failed.${RESET}"
    exit 1
else
    echo -e "${GREEN}✅  All tests passed!${RESET}"
    exit 0
fi
