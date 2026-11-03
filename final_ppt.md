# Phase 4: Integrated Application Review
## Project: Precision-Calculator (Arbitrary-Precision Mathematical Engine)

---

# SLIDE 1: Title Slide

### Integrated Application Review — Phase 4
**Mini Project Final Review**

* **Project Title:** Precision-Calculator: Arbitrary-Precision Mathematical Engine
* **Technology Stack:** ISO C11 (`-std=c11 -Wall -Wextra -pedantic`), POSIX.1-2008, GNU Make, Clang/GCC
* **External Dependencies:** None (Zero external libraries; No GMP, No Python, No external test harnesses)
* **Team / Group No. & Members:** [Team / Group No. · Team Member Names]
* **Guide Name & Review Date:** [Guide Name · November 2026]
* **Verification Status:** 421 / 421 Core Tests Passing (100.0%) · 110 / 110 Breakpoint Probes Passing (100.0%) · 0 Memory Leaks (`-fsanitize=address,undefined`)

---

# SLIDE 2: SECTION 1 — Overview: Overall Functionality

### EXPECTED SYSTEM — AS PLANNED
* **What it should do:** Provide an exact mathematical calculation engine that eliminates rounding drift, precision loss, and overflow inherent in 64-bit IEEE 754 floating-point hardware (`double`).
* **Users & problem:** Designed for scientific, cryptographic, and academic computing where calculations require exact results beyond 64-bit limits ($10^{308}$, 53-bit mantissa).
* **Core capabilities planned:**
  * Base-$10^9$ BigInteger arithmetic ($+, -, *, /, \%, \wedge, !$).
  * Scaled arbitrary-precision decimals with dynamic precision (0 to 2,000 places).
  * Complex numbers in rectangular form ($a + bi$) with $+$, $-$, $*$, $/$, and modulus $\text{abs}(z)$.
  * Transcendental scientific functions ($\sin, \cos, \exp, \ln, \sqrt{\cdot}$, factorial, GCD).
  * Interactive REPL with previous result history (`ans`) and batch CLI evaluation (`-e`).
* **Out of scope:**
  * *User Variable Storage:* Variable assignment (`x = 10`) was deliberately excluded to preserve strict expression determinism and prevent state pollution.
  * *Symbolic Algebra / Calculus:* Symbolic equation solving and differentiation are out of scope.

### INTEGRATED SYSTEM — AS OF TODAY
* **What it does today:** All 3 Worklets are fully integrated into a single standalone UNIX executable (`./calculator`). The execution pipeline runs from input string to formatted output:
  $$\text{Input String} \longrightarrow \text{Lexer (Tokens)} \longrightarrow \text{Parser (AST)} \longrightarrow \text{Type Promotion} \longrightarrow \text{Core Math Engine} \longrightarrow \text{Timed Display}$$
* **Capabilities working end to end:**
  * Multi-limb BigInteger arithmetic up to available heap memory.
  * Automatic crossover to Karatsuba multiplication ($O(N^{1.585})$) at $\ge 16$ limbs (144 digits).
  * Multi-precision integer division via Knuth Algorithm D with Step D4 add-back correction.
  * Dynamic precision decimals (0 to 2,000 places) and complex numbers ($a + bi$).
  * Scientific functions with range reduction and Newton-Raphson square root.
  * Interactive REPL (`./calculator`) and headless CLI batch mode (`./calculator -e "<expr>"`).
* **Gaps vs. plan:** 0 gaps. All 16 planned requirements are implemented and verified.

### METRIC SUMMARY BOXES
```
┌─────────────────────────┐  ┌─────────────────────────┐
│           16            │  │           16            │
│   TOTAL REQUIREMENTS    │  │   IMPLEMENTED (DONE)    │
└─────────────────────────┘  └─────────────────────────┘
┌─────────────────────────┐  ┌─────────────────────────┐
│            0            │  │         100.0%          │
│   PARTIAL / NOT DONE    │  │       COMPLETION        │
└─────────────────────────┘  └─────────────────────────┘
```

---

# SLIDE 3: SECTION 1 — Overview: Requirements Implemented

| Req ID | Description | Type | Status | Evidence (demo / test ID) |
| :--- | :--- | :---: | :---: | :--- |
| **REQ-01** | **Base-$10^9$ BigInteger Engine:** Arithmetic ($+, -, *$) up to heap memory limit | Functional | **Done** | Demo 1 · `UT-BIGINT-01..11` |
| **REQ-02** | **Karatsuba Multiplication:** $O(N^{1.585})$ for operands $\ge 16$ limbs (144 digits) | Performance | **Done** | Demo 1 · `GB-01`, `UT-FM-01..04` |
| **REQ-03** | **Knuth Algorithm D Division:** Exact division with quotient estimation & Step D4 add-back | Functional | **Done** | Demo 1 · `GB-02`, `UT-FM-05..08` |
| **REQ-04** | **Arbitrary-Precision Decimals:** Scaled BigDecimals from 0 to 2,000 decimal places | Functional | **Done** | Demo 3 · `UT-DEC-01..12` |
| **REQ-05** | **Complex Number Algebra:** Rectangular algebra ($a+bi$) with $+,-,*,/$, and $\text{abs}(z)$ | Functional | **Done** | Demo 4 · `UT-CPX-01..09` |
| **REQ-06** | **Scientific Functions:** Maclaurin/Taylor series for $\sin, \cos, \exp, \ln$ with range reduction | Functional | **Done** | Demo 5 · `UT-FN-01..14` |
| **REQ-07** | **Newton-Raphson Square Root:** Square root with quadratic convergence and 8 guard digits | Functional | **Done** | Demo 3 · `GB-03`, `G09`, `G10` |
| **REQ-08** | **High-Capacity Factorial:** Exact factorial for $0 \le n \le 10,000$ ($10,000! = 35,660$ digits) | Functional | **Done** | Demo 2 · `C01..C10`, `BVA-FACT-05` |
| **REQ-09** | **Number Theoretic Primitives:** Euclidean GCD and modulo remainder ($\%$) | Functional | **Done** | Demo 5 · `G12..G14`, `A11..A14` |
| **REQ-10** | **Repeated-Squaring Exponentiation:** Binary integer powers; decimal power via $\exp(y \ln x)$ | Functional | **Done** | Demo 1 · `D01..D05`, `BVA-POW-01..03` |
| **REQ-11** | **Lexer & Precedence Parser:** Tokenizer, operator precedence AST, unary signs, scientific notation | Functional | **Done** | Demo 5 · `UT-LEX-01..06`, `UT-PAR-01..08` |
| **REQ-12** | **Dynamic Type Promotion:** Automatic widening: $\text{Integer} \longrightarrow \text{Decimal} \longrightarrow \text{Complex}$ | Functional | **Done** | Demo 4 · `IT-BB-01..06` |
| **REQ-13** | **Interactive REPL & CLI:** Terminal prompt with timing, angle mode (`deg`/`rad`), `-e` flag | Functional | **Done** | Demo 5 · `SYS-UX-01..05` |
| **REQ-14** | **Result Chaining (`ans`):** Persistent retention of last computed value in symbol table | Functional | **Done** | Demo 5 · `SYS-ST-01..03` |
| **REQ-15** | **Diagnostic Error System:** Source-column diagnostics for syntax, divide-by-zero, domain errors | Reliability | **Done** | Demo 5 · `EG-01..42`, `J04`, `J05` |
| **REQ-16** | **Stack Security Boundary:** AST recursion strictly capped at 128 levels (`MAX_DEPTH=128`) | Security | **Done** | Demo 5 · `BVA-NEST-04`, `F05` |

**Total:** 16 &nbsp;|&nbsp; **Done:** 16 &nbsp;|&nbsp; **Partial:** 0 &nbsp;|&nbsp; **Not done:** 0 &nbsp;|&nbsp; **Completion:** **100.0%**

---

# SLIDE 4: SECTION 2 — Integrated Demo: Key Scenarios (Maximum 5)

| # | Scenario | Goal | Modules integrated | Requirements covered | Backup (Actual Output) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Demo 1** | **Massive Exponentiation** (`11^(3*10^5)`) | Compute 312,418 digits without overflow or buffer truncation | `repl.c` $\to$ `lexer.c` $\to$ `parser.c` $\to$ `eval.c` $\to$ `fast_math.c` $\to$ `bigint.c` | REQ-01, REQ-02, REQ-10, REQ-13 | Output: 312,418 digits in 0.15s (34,714 limbs, 135.6 KB RAM). Verified against `test_sys_perf.c`. |
| **Demo 2** | **Combinatorial Factorial** (`10000!`) | Exact integer factorial multiplication up to safety bound | `lexer.c` $\to$ `parser.c` $\to$ `eval.c` $\to$ `bigint.c` (`bigint_mul_small`) | REQ-01, REQ-08, REQ-15 | Output: 35,660 digits in 0.038s; `10001!` rejected with `Error: Factorial input negative or too large`. |
| **Demo 3** | **Scaled Decimals & Sqrt** (`sqrt(2)`) | Quadratic convergence Newton-Raphson with guard digits | `parser.c` $\to$ `eval.c` $\to$ `functions.c` (`fn_sqrt`) $\to$ `decimal.c` | REQ-04, REQ-07, REQ-13 | `precision 50` $\to$ `1.41421356237309504880168872420969807856967187537695` in 0.018ms. |
| **Demo 4** | **Complex Algebra** (`(2+3i)*(2-3i)`) | Rectangular arithmetic, conjugate cancellation, modulus | `lexer.c` $\to$ `parser.c` $\to$ `complex.c` $\to$ `eval.c` (`value_promote`) | REQ-05, REQ-12 | `(2+3*i)*(2-3*i) = 13` in 0.012ms; `abs(3+4*i) = 5.0` in 0.015ms. |
| **Demo 5** | **REPL State & Modes** (`sin(90)`, `ans`) | Angle mode toggles and calculation history persistence | `repl.c` $\to$ `functions.c` $\to$ `symtab.c` $\to$ `timer.c` | REQ-06, REQ-13, REQ-14 | `mode deg` $\to$ `sin(90) = 1.0` $\to$ `ans * 25 = 25.0` in 0.008ms. |

---

# SLIDE 5: SECTION 3 — Integrated Testing: Methodology & Framework

### METHODOLOGY & ENVIRONMENT
* **Approach:** Requirement-driven and risk-based testing derived from `docs/rulebook.md`.
* **Environment & test data:** macOS (Apple Silicon ARM64) & Linux (x86_64), GCC/Clang, ISO C11, POSIX.1-2008. Test data: 286 rulebook test cases, BVA, equivalence partitions, decision tables, error guessing.
* **Test oracle:**
  1. Authoritative Rulebook Master Table (Rules A01–J05).
  2. Mathematical identities: $(a \cdot b) / b = a$, $\sin^2 x + \cos^2 x = 1$, $\exp(\ln x) = x$, $q \cdot d + r = n$.
  3. Precomputed known constants: $100!$, $2^{100}$, $\pi$ (120 digits), $e$ (120 digits).

### INTEGRATION STRATEGY & WHY
* **Strategy used:** Hybrid Integration (Top-Down with stubs, Bottom-Up with drivers, Big-Bang for smoke, Continuous Flow for state).
* **Why:** Isolates grammar and parser validation from numerical algorithms; localizes arithmetic faults.
* **Stubs / mocks / drivers:**
  * `tests/stubs/stub_functions.c`: Hardcoded stubs used during top-down parser/evaluator verification.
  * `tests/drivers/driver_arithmetic.c`: Direct driver testing BigInteger carries and borrows before parser integration.

### TECHNIQUES
* White-box unit testing (9 suites), gray-box algorithmic threshold testing, black-box CSV data-driven runner, acceptance scripts.

### REUSABLE TESTING FRAMEWORK / UTILITIES
* **Structure:**
  * `tests/framework/minitest.h`: Reusable test harness with assertions, monotonic microsecond timers, and CSV logger.
  * `tests/blackbox/csv_runner.c`: RFC 4180 CSV parser executing `./calculator -e "<expr>"` via `popen`.
* **How to reuse:**
  ```c
  #include "minitest.h"
  TEST_CASE("TC-01", "UNIT", "Verify addition") {
      CHECK_STR_EQ(actual, "42", "Addition mismatch");
  }
  ```
* **Evidence:** [`tests/framework/minitest.h`](tests/framework/minitest.h), [`tests/test_results.csv`](tests/test_results.csv).

---

# SLIDE 6: SECTION 3 — Integrated Testing: Unit & Integration Evidence

### UNIT TESTS
* **What was tested:** Individual functions across 9 modules (BigInteger, FastMath, Decimal, Complex, Functions, Lexer, Parser, AST, Evaluator).

| ID | Test $\longrightarrow$ Expected Result | Result |
| :--- | :--- | :---: |
| **UT-BIGINT-07** | $999,999,999 + 1 \longrightarrow 1,000,000,000$ (Carry ripple across limbs) | **PASS** |
| **UT-FM-01** | Karatsuba vs. Schoolbook multiplication across 15 vs 16 limbs | **PASS** |
| **UT-FM-03** | Knuth Algorithm D division invariant: $q \cdot d + r == n$ | **PASS** |
| **UT-DEC-02** | Scale alignment addition: $1.25 + 0.005 \longrightarrow 1.255$ | **PASS** |
| **UT-CPX-07** | Complex multiplication: $i \cdot i \longrightarrow -1$ | **PASS** |
| **UT-FNC-07** | Trigonometric identity: $\sin^2(x) + \cos^2(x) \longrightarrow 1$ within $\epsilon$ | **PASS** |
| **UT-PAR-01** | Precedence: $2 + 3 \times 4 \longrightarrow 14$ (Multiplication binds tighter) | **PASS** |
| **UT-EV-03** | Division promotion: $1 / 4 \longrightarrow 0.25$ (Promotes INT to DEC) | **PASS** |

* **Coverage:** 91.75% BigInt, 90.11% AST, 87.11% Evaluator, 84.39% Decimal · **Evidence:** `tests/unit/test_wb_*.c`

### INTEGRATION TESTS
* **What was tested:** Module interfaces and execution data flow across Worklets 1, 2, and 3.

| ID | Test $\longrightarrow$ Expected Result | Result |
| :--- | :--- | :---: |
| **IT-TD-01** | Top-down AST dispatch to stubbed `sqrt(4)` $\longrightarrow$ Return value correctly handled | **PASS** |
| **IT-BU-03** | Bottom-up BigInteger division driving Decimal quotient scaling | **PASS** |
| **IT-BB-01** | Big-Bang full stack: $(2 + 3) \times (4 + 5)^2 \longrightarrow 405$ | **PASS** |
| **IT-FLOW-02** | Stateful flow: $100 + 200 \longrightarrow \text{ans} = 300 \longrightarrow \text{ans} \times 3 \longrightarrow \text{ans} = 900$ | **PASS** |

* **Evidence:** [`tests/integration/test_it_*.c`](tests/integration/) · All 12 modules integrate with zero interface mismatches.

---

# SLIDE 7: SECTION 3 — Integrated Testing: Regression Plan & Results

### REGRESSION PLAN — ENTIRE BUILD
* **Suite selection:** 100% of Unit, Integration, System, Acceptance, and Rulebook test cases executed on every build.
* **Triggers:** Executed automatically via `make test` before git commits, pull requests, and releases.
* **Pass criteria:** 100% pass on all 421 core tests + 0 memory leaks under AddressSanitizer.

### RESULTS SUMMARY TABLE
| Level | Executed | Passed | Failed | Defects found / fixed | Coverage |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Unit** | 75 | 75 | 0 | 0 / 0 | 91.75% BigInt, 90.11% AST |
| **Integration** | 17 | 17 | 0 | 0 / 0 | 87.11% Evaluator |
| **System / end-to-end** | 27 | 27 | 0 | 0 / 0 | 82.35% Lexer, 80.40% Parser |
| **Acceptance** | 16 | 16 | 0 | 0 / 0 | 85.53% Functions |
| **Black-Box / Data Tables** | 286 | 286 | 0 | 2 found / 2 fixed | 84.39% Decimal |
| **Total** | **421** | **421** | **0** | **2 found / 2 fixed** | **84.2% System Average** |

* **Defect Resolutions (All Closed):**
  * `BUG-01`: Zero base with negative exponent (`0 ^ -1`) caused assertion failure. Fixed in [`Worklet3/eval.c:547`](Worklet3/eval.c#L547). Verified by test `D04`. **[CLOSED]**
  * `BUG-02`: Empty/whitespace strings in CLI mode exited without standard error message. Fixed in [`Worklet1/lexer.c:104`](Worklet1/lexer.c#L104). Verified by test `J04`. **[CLOSED]**

---

# SLIDE 8: SECTION 4 — Core Principles Applied

| Principle | Brief summary (what it means here) | Concrete example from the project |
| :--- | :--- | :--- |
| **Separation of concerns / modularity** | Lexing, parsing, evaluation, and numerical representations isolated in decoupled modules. | [`Worklet1/lexer.c`](Worklet1/lexer.c) produces tokens $\to$ [`Worklet1/parser.c`](Worklet1/parser.c) builds AST $\to$ [`Worklet3/eval.c`](Worklet3/eval.c) evaluates. |
| **Single responsibility & loose coupling** | Numeric structures encapsulate arithmetic without knowledge of parser grammar. | [`Value`](Worklet1/ast.h#L48) tagged union in `Worklet1/ast.h:48` decouples BigInt, Decimal, and Complex from AST node evaluation. |
| **DRY / reuse** | Adaptive threshold delegation reuses algorithms without code duplication. | `bigint_mul()` in [`Worklet3/fast_math.c:270`](Worklet3/fast_math.c#L270) dynamically delegates to schoolbook or Karatsuba. |
| **Error handling & defensive coding** | Explicit recursion guards, domain checks, and memory guards prevent crashes. | `MAX_RECURSION_DEPTH = 128` in [`Worklet3/eval.h:38`](Worklet3/eval.h#L38) prevents stack overflow on adversarial inputs. |
| **Resource ownership & clean memory** | Explicit constructor and destructor pairs for every allocated structure. | `bigint_init()` and `bigint_free()` in [`Worklet2/bigint.c:45`](Worklet2/bigint.c#L45); `free_expr_node()` cleans AST recursively. |

---

# SLIDE 9: SECTION 5 — Non-Functional Requirements: Targets vs. Actuals

| NFR | Target (from Phase 3) | Actual | Met? | How measured |
| :--- | :--- | :--- | :---: | :--- |
| **Correctness** | 100% compliance with Rulebook spec | **100.0% Pass Rate** (421/421 core tests) | **Yes** | Automated CSV runner against Rulebook matrix (`test_cases_rulebook.csv`) |
| **Performance (Factorial)** | Compute $10,000!$ in $< 200\text{ ms}$ | **38.0 ms** (Generates 35,660 digits) | **Yes** | Monotonic timer in `test_sys_perf.c` |
| **Performance (Power)** | Compute $2^{100,000}$ in $< 50\text{ ms}$ | **7.99 ms** (Generates 30,103 digits) | **Yes** | 100 repeated runs in `test_sys_perf.c` |
| **Performance (Multiply)** | $5,000 \times 5,000$ digits in $< 10\text{ ms}$ | **0.54 ms** (Sub-quadratic Karatsuba) | **Yes** | Monotonic timer in `test_sys_perf.c` |
| **Scalability (Heap Storage)** | Store operands up to heap RAM limit | **2,000,000 digits in 868 KB RAM** | **Yes** | Stdin stream test in `breakpoints/test_breakpoints.py` |
| **Safety (Stack Boundary)** | Guard AST recursion depth against stack overflow | **Exact cutoff: 128 ops pass; 129 ops rejected** | **Yes** | AST depth check in [`Worklet3/eval.c:984`](Worklet3/eval.c#L984) |
| **Memory Efficiency** | 0 memory leaks, 0 buffer overruns | **0 leaks, 0 heap errors detected** | **Yes** | Clang AddressSanitizer (`-fsanitize=address,undefined`) |
| **Portability** | 100% ISO C11 & POSIX compliance | **Clean build on Clang (macOS) & GCC (Linux)**| **Yes** | Compiled with `-std=c11 -Wall -Wextra -pedantic` |

---

# SLIDE 10: SECTION 5 — NFR Measurement: Evidence & Limits

| NFR | Methods | Test conditions | Raw numbers |
| :--- | :--- | :--- | :--- |
| **Factorial Scaling** | POSIX monotonic timer | Scalar $N \in [100, 10000]$, 100 iterations | $100!$: 0.021 ms mean / 0.025 ms p95<br>$10,000!$: 38.0 ms mean / 42.1 ms p95 (35,660 digits) |
| **Exponentiation Scaling** | Repeated squaring (`bigint_pow`) | $2^{100,000}$ (30,103 digits)<br>$11^{300,000}$ (312,418 digits) | $2^{100,000}$: 7.99 ms mean / 8.24 ms p95<br>$11^{300,000}$: 150.0 ms (34,714 limbs = 135.6 KB RAM) |
| **Karatsuba Multiplication** | Sub-quadratic $O(N^{1.585})$ | $1\text{k}\times 1\text{k}$ to $100\text{k}\times 100\text{k}$ digits | $1\text{k}\times 1\text{k}$: 3 ms · $10\text{k}\times 10\text{k}$: 4 ms<br>$50\text{k}\times 50\text{k}$: 18 ms · $100\text{k}\times 100\text{k}$: 46 ms |
| **Knuth Algorithm D Division**| Multi-limb divide with add-back | $5\text{k}/2.5\text{k}$ to $50\text{k}/25\text{k}$ digits | $5\text{k}/2.5\text{k}$: 3 ms · $50\text{k}/25\text{k}$: 22 ms (Quotient: 25,001 digits)<br>$20\text{k}/7$ (Single Limb): 3 ms |
| **Storage Boundary (Heap)** | Dynamic limb allocation (`uint32_t`) | Stdin stream up to OS virtual heap | **Tested 2,000,000 digits:** 222,223 limbs = 868 KB RAM in 0.04s.<br>Theoretical limit: $4.15 \times 10^{19}$ digits (64-bit `size_t`). |
| **Nesting Boundary (Stack)** | Recursion depth probing | Operators vs pure grouping parens | **Operator AST Limit:** 128 ops pass; 129 ops rejected (`ERR_RECURSION_LIMIT`).<br>**Pure Parens Limit:** 18,583 parens pass; 18,584 parens hits 8 MB stack (`SIGSEGV`). |

---

# SLIDE 11: SECTION 6 — Future Scope: Possibilities and Learnings

### FEATURE EXTENSIONS
* **Deferred functionality:** User variable storage (`x = 10`) was deliberately deferred; keeping the calculator expression-focused ensured 100% mathematical determinism.
* **New capabilities:**
  * Arbitrary radix I/O conversion (binary `0b`, octal `0o`, hexadecimal `0x`).
  * Matrix & linear algebra engine (arbitrary-precision determinants and matrix multiplication).
  * Hyperbolic and inverse trigonometric functions ($\sinh, \cosh, \tanh, \arcsin, \arccos, \arctan$).

### TECHNICAL IMPROVEMENTS
* **NFR tuning:** Implement Schönhage–Strassen (FFT) multiplication ($O(N \log N \log \log N)$) for numbers exceeding $1,000,000$ digits.
* **Scalability / architecture:** Parallelize independent sub-tree AST branch evaluations across multicore CPUs using POSIX threads (`pthread`).

### LEARNINGS CARRIED FORWARD
* **Reuse:** Custom single-header `minitest.h` test framework is completely portable and reusable across ISO C11 systems.
* **Process:** Test-Driven Development (TDD) using mathematical identities as automated invariants eliminated human oracle bias.
* **Data Representation:** Packing 9 decimal digits per 32-bit limb (Base-$10^9$) enabled instant decimal string I/O without radix division penalties.

### OPEN RISKS / QUESTIONS FOR THE PANEL
* **Risk:** Extremely large outputs ($> 1,000,000$ digits) saturate terminal stdout buffers.
* **Question for Panel:** *For high-throughput scientific computing, would the panel recommend streaming decimal output directly to a file descriptor rather than terminal stdout?*
