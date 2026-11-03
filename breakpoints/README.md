# Precision-Calculator: Extremities & Breakpoints Suite

This directory contains the automated probing harness, empirical benchmark logs, and architectural analysis for all operational limits, memory thresholds, and breakpoints of the **Precision-Calculator** mathematical engine.

---

## Directory Contents

| File | Role / Purpose |
| :--- | :--- |
| **[`test_breakpoints.py`](test_breakpoints.py)** | Standalone Python automated probing suite. Tests 110 distinct extremity vectors across 16 categories. |
| **[`runner.sh`](runner.sh)** | Shell script to compile the calculator (if needed) and run the full extremity probing harness. |
| **[`BREAKPOINTS_ANALYSIS.md`](BREAKPOINTS_ANALYSIS.md)** | **Exhaustive 20-category report.** Catalogs every breakpoint, safe limit, failure threshold, empirical outputs, and 2-3 line technical explanations referencing the C source code. |
| **[`test_results.csv`](test_results.csv)** | Machine-readable empirical test execution log containing timings, exit codes, and output comparisons for all 110 probes. |

---

## How to Execute the Breakpoint Suite

From the project root directory, run:

```bash
# Using the shell runner script:
./breakpoints/runner.sh

# Or directly with Python:
python3 breakpoints/test_breakpoints.py
```

---

## Key Breakpoint Discoveries at a Glance

1. **Linear Expression Length:** Breaks at **129 additions (130 operands)** with `Error: Recursion depth limit exceeded` due to left-associative AST depth exceeding `MAX_RECURSION_DEPTH = 128` ([`Worklet3/eval.h:38`](../Worklet3/eval.h#L38)).
2. **Balanced Expression Trees:** Successfully evaluates **10,000+ operands** because balanced binary tree depth ($\lceil \log_2(10000) \rceil = 14$) stays far below the 128-level limit.
3. **Operand Digit Storage:** Handles **1,000,000+ digits** without truncation using Base-$10^9$ limbs dynamically expanded via `realloc()`.
4. **Grouping Parentheses:** Handles up to **18,580 nested parentheses** `(((42)))` before hitting the OS 8 MB C thread stack guard page (`SIGSEGV`).
5. **Factorial Limit:** Calculates exact **$10,000!$ (35,660 digits)** in ~50 ms; rejects **$10,001!$** cleanly in $<0.01$ ms via safety guard `Worklet3/eval.c:437`.
6. **Decimal Precision:** Allows dynamic precision from **0 up to 2,000 decimal places**; rejects $p = 2,001$ to prevent quadratic Taylor series latency.
7. **Command Line Argument Length:** Limited to **1,048,576 bytes (1 MB)** by macOS POSIX `ARG_MAX`; inputs exceeding 1 MB must be piped via standard input (`stdin`).

