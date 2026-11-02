# Master Test Plan & Quality Assurance Strategy

**Project Name:** Precision-Calculator (Arbitrary-Precision Computing Engine)  
**Standard:** ISO C11, IEEE 829-2008 Test Documentation Standard, POSIX.1-2008  
**Document Identifier:** DOC-TP-2026-01  
**Author:** Senior QA & C Systems Engineering Lead  
**Classification:** Quality Assurance & System Verification Plan  

---

## 1. About Software Testing

Software Testing for **Precision-Calculator** is a rigorous verification and validation process designed to guarantee that the native computational engine matches all functional requirements specified in the authoritative [rulebook.md](file:///Users/makarandkulkarni/Myspace/Mini-project/Precision-Calculator/docs/rulebook.md), Low-Level Design (`LLD_Technical_Design.md`), and High-Level Design (`HLD_System_Architecture.md`).

### 1.1 Purpose
The fundamental objectives of this testing suite are:
1. **Identify Defects, Gaps, and Inconsistencies:** Expose logic errors, numerical instability, memory leaks, buffer overruns, and divergences from the Rulebook specification.
2. **Ensure Non-Functional Correctness:** Verify the core software attributes:
   - **Reliability:** Exact arithmetic invariants, zero undefined behavior, determinism across millions of cycles.
   - **Scalability:** Unbounded limb storage up to 100,000+ decimal digits, Karatsuba sub-quadratic multiplication scaling ($O(N^{1.585})$).
   - **Portability:** Conformance to pure ISO C11 and POSIX.1-2008 across 32-bit/64-bit systems, Clang, GCC, and diverse system locales.
   - **Reusability:** Clean encapsulation of Worklet 2 (`BigInteger`) and Worklet 3 (`BigDecimal`, `Complex`) as embeddable libraries.
   - **Usability:** Intuitive interactive REPL interface, clear syntax-error diagnostics, helpful error messages, and sub-millisecond execution telemetry.
3. **Mandatory Deployment Gate:** Software cannot be deployed or integrated into downstream applications without passing the automated testing suite and memory sanitizers with zero regressions.
4. **Lifecycle Coverage:** Testing is conducted continuously at every Software Development Life Cycle (SDLC) phase:
   - *Requirement Analysis:* Rulebook verification and equivalence class identification.
   - *Design Phase:* Architecture review and interface contract definition.
   - *Coding Phase:* Developer unit testing and static analysis (`-Wall -Wextra -pedantic`).
   - *Testing Phase:* Automated integration, system, performance, and fuzz testing.
   - *Maintenance Phase:* Automated regression gates and defect tracking.

### 1.2 Stakeholder Roles & Responsibilities

| Role | Responsibilities | Key Deliverables & Activities |
| :--- | :--- | :--- |
| **Software Developer** | Implements algorithms, conducts static code inspection, executes white-box unit tests, and maintains memory invariants. | Module unit tests (`tests/unit/`), memory cleanup (`valgrind`, `ASan`). |
| **Software Tester / QA Engineer** | Designs black-box test matrices, executes automated test pipelines, conducts exploratory & gray-box tests, tracks defects. | Integration suites, Master Rulebook runner (`tests/blackbox/`), `DEFECT_LOG.md`. |
| **Project Manager** | Analyzes residual risk, prioritizes defect triage, monitors defect clustering (Pareto 80/20), approves release criteria. | Sign-off approval, milestone tracking, resource allocation. |
| **End-User / Client** | Conducts User Acceptance Testing (UAT) based on scientific workflows; verifies usability and operational performance. | Acceptance test scenarios, usability survey completion. |

---

## 2. Taxonomy & Types of Software Testing

The Precision-Calculator testing suite is structured around a complete testing taxonomy:

```
Testing Hierarchy
├── Manual Testing
│   └── Visual Inspection & Checklist (manual_checklist.md)
│       ├── REPL Look-and-Feel, Prompt rendering ("> ")
│       ├── Help Banner & Command Cheat Sheet completeness
│       └── Syntax Error Caret (^) alignment verification
├── Automation Testing (Makefile test targets)
│   ├── White-Box Testing (Code-based, developer view)
│   │   ├── Statement Coverage (gcov target >= 90%)
│   │   ├── Branch & Condition Coverage (carry/borrow, Knuth add-back)
│   │   ├── Path Coverage & Cyclomatic Complexity (eval_node, parser)
│   │   └── Loop Testing (zero, one, typical, max iterations)
│   ├── Gray-Box Testing
│   │   └── Public interface driving internal threshold switchovers
│   └── Black-Box Testing (Specification-based, tester view)
│       ├── Functional Testing
│       │   ├── Unit Testing (per-module isolation)
│       │   ├── Integration Testing (Top-Down, Bottom-Up, Big-Bang)
│       │   ├── System Testing (End-to-End CLI / REPL modes)
│       │   └── User Acceptance Testing (UAT, BAT, RAT)
│       └── Non-Functional Testing
│           ├── Performance & Benchmark Latency Testing
│           ├── Load & Thread Concurrency Testing
│           ├── Stress & LCG Pseudo-Random Fuzz Testing
│           ├── Recovery & Fault Isolation Testing
│           ├── Usability Testing
│           └── Compatibility & Migration Testing (GCC/Clang, 32/64-bit)
```

---

## 3. The 7 Testing Principles Applied

| Principle | Core Philosophy | Precision-Calculator Concrete Artifact |
| :---: | :--- | :--- |
| **1. Testing Shows Presence of Defects** | Testing demonstrates the presence of defects, not their absence. The software is never declared 100% bug-free. | `docs/QUALITY_REPORT.md` documents residual risk and bounded guarantees rather than claiming absolute absence of bugs. |
| **2. Exhaustive Testing is Impossible** | The input space (infinite digit strings, arbitrarily nested expressions) cannot be exhaustively tested. Risk analysis guides test priority. | Risk-based prioritization: Critical modules (Knuth D, Karatsuba, parser precedence, memory lifecycle) receive priority over minor cosmetic strings. |
| **3. Early Testing** | Detecting errors early in the SDLC drastically reduces remediation cost and structural rework. | Independent unit test suites run for Worklets 1, 2, and 3 using mock stubs before full integration. |
| **4. Defect Clustering (Pareto 80/20)** | 80% of defects cluster within 20% of modules (historically Knuth Algorithm D division and parser error recovery). | `docs/DEFECT_LOG.md` maintains a defect density breakdown per module to focus testing on high-risk clusters. |
| **5. Pesticide Paradox** | Repeating identical static tests ceases to uncover new edge cases. | An integrated Linear Congruential Generator (LCG) generates randomized expressions and byte fuzzing with deterministic seeds on every run. |
| **6. Testing is Context-Dependent** | A high-precision numeric engine requires specialized testing (limb boundaries, guard digits, scale alignment) unlike typical Web/CRUD applications. | Custom boundary tests: Base-$10^9$ limits ($10^9-1$, $10^9$), Karatsuba limb threshold (32 limbs), and Taylor series epsilon convergence. |
| **7. Absence-of-Errors Fallacy** | A system passing all unit tests is useless if it fails to solve user requirements or outputs unreadable formats. | Acceptance tests (`tests/acceptance/test_uat_scenarios.sh`) validate genuine scientist and student computation workflows and latency bounds. |

---

## 4. Test Levels & Methodologies

### 4.1 Unit Testing (Level 1)
- **Scope:** Every individual C compilation unit tested in isolation:
  - Worklet 1: `lexer.c`, `parser.c`, `ast.c`, `bigint_io.c`, `timer.c`, `repl.c`
  - Worklet 2: `bigint.c`
  - Worklet 3: `fast_math.c`, `decimal.c`, `complex.c`, `functions.c`, `eval.c`
- **Techniques:** White-box structural testing (statement, branch, path, loop) + algebraic identity validation (e.g., $(a \times b) / b = a$, $\sin^2(x) + \cos^2(x) = 1$).
- **Object-Oriented Invariant Testing:** Testing internal data structures via public APIs to confirm:
  - All limb values satisfy $0 \le \text{digits}[k] < 10^9$.
  - Canonical zero invariant: `sign = 0, size = 1, digits[0] = 0`.
  - Deep-copy semantics: `symtab_set()` duplicates strings and values independently.

### 4.2 Integration Testing (Level 2)
- **Scope:** Correctness and data flow between interacting modules:
  - Lexer $\to$ TokenList $\to$ Parser $\to$ AST $\to$ Evaluator $\to$ BigDecimal $\to$ BigInteger $\to$ Serializer.
- **Approaches:**
  1. **Incremental Top-Down:** Starts at `eval.c` / `repl.c`, using Stubs (`tests/stubs/`) to emulate missing lower-level math engines.
  2. **Incremental Bottom-Up:** Starts at `bigint.c` and builds upward through `fast_math.c` and `decimal.c` using Drivers (`tests/drivers/`).
  3. **Non-Incremental (Big-Bang):** Links all modules into a unified binary and executes complete end-to-end calculation workflows.
  4. **Stateful Flow Testing:** Mirrors the "Login $\to$ Compose $\to$ Logout" session analog:
     $$\text{Expression 1} \xrightarrow{\text{Eval}} \text{Store to ans} \xrightarrow{\text{Expression 2}} \text{Read ans} \xrightarrow{\text{Result}}$$

### 4.3 System Testing (Level 3)
Evaluates the fully compiled `./calculator` binary across 9 distinct categories:
1. **Performance:** Sub-second requirement for $100\text{-digit} \div 200\text{-digit}$ division; benchmarks latency of $2^{100000}$, $10000!$, $\sqrt{2}$, $\pi$, and $e$.
2. **Load & Concurrency:** 100,000 piped sequential lines in REPL; multithreaded evaluation with POSIX `pthread` verifying thread isolation.
3. **Usability:** Help banner completeness, error message clarity, syntax error caret pointers.
4. **Regression:** Compares outputs against `tests/data/golden_results.txt`.
5. **Migration:** Evaluates compilation under GCC, Clang, `-O0`, `-O2`, `-O3`, and alternate locales.
6. **Functional:** Comprehensive coverage of Rulebook operations and missing-function rejection (e.g. `tan`, `asin`).
7. **Recovery:** Verifies error recovery where errors reset state cleanly and leave `ans` intact.
8. **Stress & Fuzzing:** Deep parenthetical nesting ($> 128$), $100{,}000$-digit literals, and deterministic LCG fuzzing.
9. **Hardware/Software Compatibility:** Endianness independence, word-size assumptions (`sizeof(uint32_t) == 4`, `sizeof(uint64_t) == 8`), and zero dynamic dependency check via `ldd`.

### 4.4 Acceptance Testing (Level 4)
- **User Acceptance Testing (UAT):** Realistic calculations from an engineering student and scientific researcher point of view.
- **Business Acceptance Testing (BAT):** Verifies standalone zero-dependency build, CLI scripting flags (`-e`), and exit status codes.
- **Regulations Acceptance Testing (RAT):** Strict ISO C11 (`-std=c11 -pedantic`) and POSIX.1-2008 compliance.
- **Alpha vs. Beta Testing:** Internal comprehensive testing (Alpha) vs. realistic user transcript replay in a clean environment (Beta).

---

## 5. Verification vs. Validation Strategy

| Dimension | Verification (Static) | Validation (Dynamic) |
| :--- | :--- | :--- |
| **Guiding Question** | *"Are we building the product right?"* | *"Have we built the right product?"* |
| **Activity Type** | Static code inspection, design walkthroughs, checklist audits, compiler warning checks. | Dynamic test execution, automated test runner execution, performance benchmarking. |
| **Executed By** | Developers, QA Lead, System Architects. | Testers, End Users, Continuous Integration harness. |
| **Precision-Calculator Scope** | Documented in `docs/verification_review.md`: Checking structural constants (`KARATSUBA_THRESHOLD=32`, `MAX_RECURSION_DEPTH=128`, limb constraints). | Executed test suites: Unit, Integration, System, Blackbox, UAT. |

---

## 6. Testing vs. Debugging Strategy

| Attribute | Software Testing | Software Debugging |
| :--- | :--- | :--- |
| **Objective** | Uncover the presence of bugs, defects, and requirement deviations. | Identify the root cause of a defect and implement corrective code fixes. |
| **Primary Actor** | Software Tester / QA Automation Engineer. | Software Developer / Systems Programmer. |
| **Methodology** | Automated test harnesses, fuzzing, black-box matrices, boundary analysis. | Interactive debuggers (`gdb`, `lldb`), logging, memory sanitizers (`ASan`, `Valgrind`). |
| **Plannability** | Fully plannable, structured, and repeatable. | Unplannable; duration and complexity depend entirely on the bug nature. |
| **Policy in this Suite** | **Zero Modification of Production Code:** Discrepancies and failures are recorded into `docs/DEFECT_LOG.md` with file/line localization and hypothesis notes without altering production logic. |

---

## 7. Test Deliverables & Execution Summary

- **Test Framework:** `tests/framework/minitest.h`
- **Test Automation Runner:** `Makefile` target `make test`
- **Traceability Matrix:** `docs/TRACEABILITY.md`
- **Defect Tracking Log:** `docs/DEFECT_LOG.md`
- **Quality & Metrics Report:** `docs/QUALITY_REPORT.md`
- **Maintenance Strategy Plan:** `docs/MAINTENANCE_PLAN.md`
- **Verification Review:** `docs/verification_review.md`
- **Manual Usability Checklist:** `docs/manual_checklist.md`
- **Discrepancy Matrix:** `docs/spec_discrepancies.md`

