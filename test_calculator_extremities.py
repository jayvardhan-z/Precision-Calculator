#!/usr/bin/env python3
"""
Precision Calculator Extremity Benchmark
========================================
Discovers practical limits by increasing one dimension at a time, then runs
combined stress cases. Intended for a calculator executable that accepts:
    ./calculator "1+2"
and prints a result line such as:
    = 3 [0.001 ms]

This measures limits of the built binary on the current machine/configuration.
A reported maximum is only a confirmed lower bound unless the next size fails.
"""

from __future__ import annotations
import argparse
import csv
import json
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path

DEFAULT_FUNCTIONS = [
    "sqrt", "abs", "sin", "cos", "tan", "asin", "acos", "atan",
    "sinh", "cosh", "tanh", "log", "log10", "ln", "exp",
    "floor", "ceil", "round", "conj", "real", "imag", "arg",
]

RESULT_RE = re.compile(r"=\s*(.*?)\s*(?:\[[^\]]*\])?\s*$", re.MULTILINE)

@dataclass
class Outcome:
    category: str
    size: int
    status: str
    returncode: int | None
    elapsed_seconds: float
    output_chars: int
    detail: str = ""

def parse_result(stdout: str):
    # Prefer the final calculator result line. Don't accidentally count an
    # echoed input line as a result.
    matches = list(RESULT_RE.finditer(stdout))
    if matches:
        value = matches[-1].group(1).strip()
        if value and not value.lower().startswith(("error", "invalid")):
            return value
    # Generic fallback for calculator variants with a bare result output.
    lines = [x.strip() for x in stdout.splitlines() if x.strip()]
    for line in reversed(lines):
        if not line.startswith((">", "Error:", "error:", "Invalid", "invalid")):
            if not re.search(r"(recursion depth limit|syntax error|division by zero|domain error)", line, re.I):
                return line
    return None

def run_expr(binary: str, expr: str, timeout: float, expected: str | None = None):
    started = time.monotonic()
    try:
        p = subprocess.run(
            [binary, expr], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=timeout
        )
        elapsed = time.monotonic() - started
        out = p.stdout or ""
        combined = (out + "\n" + (p.stderr or "")).strip()
        low = combined.lower()
        if p.returncode != 0:
            status = "NONZERO_EXIT"
            detail = combined[-300:].replace("\n", " ")
        elif any(word in low for word in ("recursion depth limit exceeded", "stack overflow", "segmentation fault")):
            status = "LIMIT_OR_CRASH"
            detail = combined[-300:].replace("\n", " ")
        elif any(word in low for word in ("error:", "syntax error", "invalid expression", "domain error")):
            status = "ERROR_OUTPUT"
            detail = combined[-300:].replace("\n", " ")
        else:
            result = parse_result(out)
            if result is None:
                status = "NO_RESULT"
                detail = combined[-300:].replace("\n", " ")
            elif expected is not None:
                # Exact comparison after trimming. Formatting-specific cases
                # (e.g. decimals) should be treated as unchecked if formatting differs.
                if result == expected:
                    status, detail = "PASS_CHECKED", ""
                else:
                    status, detail = "WRONG_RESULT", f"expected={expected[:100]!r}; got={result[:100]!r}"
            else:
                status, detail = "PASS_UNCHECKED", f"result={result[:100]!r}"
        return status, p.returncode, elapsed, len(out), detail
    except subprocess.TimeoutExpired:
        return "TIMEOUT", None, time.monotonic() - started, 0, f"exceeded {timeout}s"
    except OSError as e:
        return "LAUNCH_ERROR", None, time.monotonic() - started, 0, str(e)

def all_nines_plus_one(digits: int):
    # 999...999 + 1 = 1000...000 (digits zeros); no Python int conversion.
    return "9" * digits + "+1", "1" + ("0" * digits)

def write_reports(rows, outdir: Path):
    outdir.mkdir(parents=True, exist_ok=True)
    with (outdir / "extremity_report.csv").open("w", newline="", encoding="utf-8") as f:
        fields = list(asdict(rows[0]).keys()) if rows else [
            "category","size","status","returncode","elapsed_seconds","output_chars","detail"
        ]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for row in rows:
            w.writerow(asdict(row))
    with (outdir / "extremity_report.json").open("w", encoding="utf-8") as f:
        json.dump([asdict(x) for x in rows], f, indent=2)

def measure(rows, binary, timeout, category, size, expr, expected=None):
    status, rc, elapsed, outchars, detail = run_expr(binary, expr, timeout, expected)
    row = Outcome(category, size, status, rc, round(elapsed, 4), outchars, detail)
    rows.append(row)
    print(f"{category:30} size={size:8} -> {status:16} {elapsed:.3f}s")
    return status in ("PASS_CHECKED", "PASS_UNCHECKED")

def exponential_then_binary(rows, binary, timeout, category, make_case, start, ceiling):
    """Find highest passing size by exponential growth then binary search.
    Pass means a result is produced with exit code 0 and no recognized error.
    """
    tested = {}
    def check(n):
        if n not in tested:
            expr, expected = make_case(n)
            tested[n] = measure(rows, binary, timeout, category, n, expr, expected)
        return tested[n]
    if not check(start):
        return 0, "fails at starting size"
    lo, hi = start, start
    while hi < ceiling:
        nxt = min(ceiling, hi * 2)
        if nxt == hi:
            break
        if check(nxt):
            lo = nxt
            hi = nxt
        else:
            hi = nxt
            break
    if lo == ceiling and tested.get(ceiling):
        return lo, "ceiling reached; true maximum not established"
    # If initial doubling found a failure, binary search between last pass and failure.
    fail_candidates = [n for n, ok in tested.items() if not ok and n > lo]
    if not fail_candidates:
        return lo, "no failing size found"
    hi = min(fail_candidates)
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if check(mid):
            lo = mid
        else:
            hi = mid
    return lo, f"next tested failure at {hi}"

def main():
    ap = argparse.ArgumentParser(description="Find practical extremity limits of a calculator binary.")
    ap.add_argument("--binary", default="./calculator", help="calculator executable (default: ./calculator)")
    ap.add_argument("--timeout", type=float, default=3.0, help="timeout per test in seconds")
    ap.add_argument("--ceiling", type=int, default=100000, help="maximum size attempted per dimension")
    ap.add_argument("--outdir", default="tests/extremity_reports", help="report directory")
    ap.add_argument("--functions", default=",".join(DEFAULT_FUNCTIONS),
                    help="comma-separated function names to probe; customize to your calculator")
    ap.add_argument("--quick", action="store_true", help="smaller ceilings and fewer combined tests")
    args = ap.parse_args()

    binary = args.binary
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print(f"ERROR: executable not found or not executable: {binary}", file=sys.stderr)
        print("Build first, e.g. make clean && make", file=sys.stderr)
        sys.exit(2)

    ceiling = min(args.ceiling, 10000) if args.quick else args.ceiling
    rows = []
    summaries = []

    print("\n=== 1. Maximum operand digits (verified: all-9s + 1) ===")
    result, note = exponential_then_binary(
        rows, binary, args.timeout, "max_operand_digits",
        lambda n: all_nines_plus_one(n), 8, ceiling
    )
    summaries.append(("max_operand_digits", result, note))

    print("\n=== 2. Maximum expression length: long addition chain ===")
    def chain_case(n):
        terms = max(2, (n + 1) // 2)
        expr = "+".join(["1"] * terms)
        return expr, str(terms)
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_addition_chain_terms",
                                          chain_case, 4, max(4, ceiling // 2))
    summaries.append(("max_addition_chain_terms", result, note))

    print("\n=== 3. Maximum expression character length: long unary-free chain ===")
    def expr_len_case(n):
        # Keep expression length close to n using a repeated '1+1+...'.
        terms = max(2, (n + 1) // 2)
        expr = "+".join(["1"] * terms)
        return expr, str(terms)
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_expression_chars_chain",
                                          expr_len_case, 7, ceiling)
    summaries.append(("max_expression_chars_chain", result, note))

    print("\n=== 4. Expression with multiple maximum-length operands ===")
    # Test two huge operands in a single addition; expected is checked only for
    # moderate sizes to avoid constructing an unwieldy expected string repeatedly.
    def multi_operand_case(n):
        a = "9" * n
        b = "1" + ("0" * (n - 1))
        expected = None
        if n <= 5000:
            # 99...99 + 10...00 = 10...099...99; compute by string arithmetic
            # using a simple digit-wise addition helper below.
            expected = decimal_add(a, b)
        return f"{a}+{b}", expected
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_two_operand_digits_each",
                                          multi_operand_case, 8, max(8, ceiling // 2))
    summaries.append(("max_two_operand_digits_each", result, note))

    print("\n=== 5. Maximum parenthesis nesting ===")
    def paren_case(n):
        return "(" * n + "1" + ")" * n, "1"
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_parenthesis_depth",
                                          paren_case, 2, min(ceiling, 20000))
    summaries.append(("max_parenthesis_depth", result, note))

    print("\n=== 6. Maximum nested function depth (support-dependent) ===")
    # sqrt nested around 1 is mathematically 1 if sqrt is supported.
    def func_case(n):
        return "sqrt(" * n + "1" + ")" * n, None
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_sqrt_nesting_depth",
                                          func_case, 1, min(ceiling, 20000))
    summaries.append(("max_sqrt_nesting_depth", result, note))

    print("\n=== 7. Function support smoke tests ===")
    for fn in [x.strip() for x in args.functions.split(",") if x.strip()]:
        # Values selected to be valid for common functions; some functions may
        # have different signatures or domain requirements in this project.
        arg = "1"
        expr = f"{fn}({arg})"
        measure(rows, binary, args.timeout, f"function_support:{fn}", 1, expr, None)

    print("\n=== 8. Unary operator chain ===")
    def unary_case(n):
        # Even number of minus signs produces +1; odd produces -1.
        signs = "-" * n
        expected = "1" if n % 2 == 0 else "-1"
        return signs + "1", expected
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_unary_minus_count",
                                          unary_case, 1, ceiling)
    summaries.append(("max_unary_minus_count", result, note))

    print("\n=== 9. Decimal literal length ===")
    def decimal_case(n):
        # Check acceptance only: output formatting/precision may differ.
        return "0." + ("1" * n) + "+0", None
    result, note = exponential_then_binary(rows, binary, args.timeout, "max_decimal_fraction_digits",
                                          decimal_case, 8, min(ceiling, 50000))
    summaries.append(("max_decimal_fraction_digits", result, note))

    print("\n=== 10. Scientific notation exponent range ===")
    # This is a boundary sweep, not a binary search: some calculators clamp,
    # overflow, or support arbitrary exponent parsing.
    for exp in [1, 2, 10, 100, 1000, 10000, 100000, 1000000]:
        measure(rows, binary, args.timeout, "scientific_exponent", exp, f"1e{exp}", None)
        measure(rows, binary, args.timeout, "scientific_negative_exponent", exp, f"1e-{exp}", None)

    print("\n=== 11. Function argument / domain checks ===")
    for expr in ["sqrt(0)", "sqrt(-1)", "log(1)", "log(0)", "log(-1)",
                 "1/0", "0/0", "(-1)^0.5", "sin(0)", "cos(0)", "tan(0)"]:
        measure(rows, binary, args.timeout, "domain_and_boundary", len(expr), expr, None)

    print("\n=== 12. Combined stress tests ===")
    combined_ceiling = min(ceiling, 20000)
    combos = [
        ("combined_big_operands", ("9" * min(combined_ceiling // 4, 5000)) + "+(" +
         ("8" * min(combined_ceiling // 4, 5000)) + ")", None),
        ("combined_many_terms_and_parens", "(" * 100 + "+".join(["1"] * 101) + ")" * 100, None),
        ("combined_nested_functions_and_chain", "sqrt(" * 50 + "(" + "+".join(["1"] * 51) + ")" + ")" * 50, None),
        ("combined_long_unary_and_parens", "(" * 100 + "-" * 100 + "1" + ")" * 100, None),
    ]
    if not args.quick:
        combos.extend([
            ("combined_1000_operator_chain", "+".join(["1"] * 1001), "1001"),
            ("combined_5000_operator_chain", "+".join(["1"] * 5001), "5001"),
        ])
    for name, expr, expected in combos:
        measure(rows, binary, args.timeout, name, len(expr), expr, expected)

    print("\n=== 13. Invalid-input resilience ===")
    invalids = [
        "", "+", "1+", "*1", "()", "(", ")", "1..2", "1**2",
        "sqrt(", "unknownFunction(1)", "1//2", "1 + @", "((((1))))))",
    ]
    for expr in invalids:
        measure(rows, binary, args.timeout, "invalid_input", len(expr), expr, None)

    outdir = Path(args.outdir)
    write_reports(rows, outdir)
    print("\n=== SUMMARY ===")
    for name, value, note in summaries:
        print(f"{name:34} best_size={value:<8} {note}")
    print(f"\nDetailed reports:\n  {outdir / 'extremity_report.csv'}\n  {outdir / 'extremity_report.json'}")
    print("Interpretation: PASS_UNCHECKED means accepted/output produced, not mathematically verified.")
    print("A ceiling result means the benchmark did not prove the actual maximum.")

def decimal_add(a: str, b: str) -> str:
    i, j, carry = len(a) - 1, len(b) - 1, 0
    out = []
    while i >= 0 or j >= 0 or carry:
        x = ord(a[i]) - 48 if i >= 0 else 0
        y = ord(b[j]) - 48 if j >= 0 else 0
        s = x + y + carry
        out.append(chr(48 + (s % 10)))
        carry = s // 10
        i -= 1
        j -= 1
    return "".join(reversed(out))

if __name__ == "__main__":
    main()
