# Requirements & Syllabus Traceability Matrix

**Project:** Precision-Calculator  
**Standard:** IEEE 829-2008 / ISO C11  
**Classification:** Quality Assurance & Verification Traceability  

This document establishes bidirectional traceability between:
1. **The Course Syllabus (`test_guide.md`):** Every theoretical and practical testing concept mapped to executable tests and documentation.
2. **The Authoritative Rulebook (`rulebook.md`):** Every grammar rule, operator, function, boundary, and error diagnostic mapped to specific test cases.

---

## 1. Syllabus Topic Traceability (test_guide.md)

| Syllabus Section | Topic Description | Implementation File / Target | Test Suite / Script | Primary Test IDs | Status |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **§1** | **About Software Testing** | `docs/TEST_PLAN.md` §1 | Documentation & Lifecycle review | DOC-TP-01 | PASS |
| **§2** | **Types of Software Testing** | `docs/TEST_PLAN.md` §2, `docs/manual_checklist.md` | Manual checklist & Automated hierarchy | TAXONOMY-01 | PASS |
| **§3** | **Test Case Template** | `docs/TEST_CASES.md`, `tools/generate_test_cases_doc.sh` | Automated template generation from results | TC-TMPL-01 | PASS |
| **§4.1** | **Principle 1: Testing Shows Presence of Defects** | `docs/QUALITY_REPORT.md` §4 | Residual risk & defect disclosures | PRIN-01 | PASS |
| **§4.2** | **Principle 2: Exhaustive Testing is Impossible** | `docs/TEST_PLAN.md` §3 | Risk-based prioritization strategy | PRIN-02 | PASS |
| **§4.3** | **Principle 3: Early Testing** | `tests/unit/`, `tests/stubs/` | Worklet unit tests with mock stubs | PRIN-03, UT-STUB-01 | PASS |
| **§4.4** | **Principle 4: Defect Clustering** | `docs/DEFECT_LOG.md` §2 | Pareto 80/20 module defect clustering | PRIN-04 | PASS |
| **§4.5** | **Principle 5: Pesticide Paradox** | `tests/system/test_sys_stress_fuzz.c` | Seeded LCG dynamic expression fuzzer | PRIN-05, ST-FUZZ-01 | PASS |
| **§4.6** | **Principle 6: Context Dependent** | `tests/data/test_cases_bva.csv` | Numerical engine limb boundary tests | PRIN-06, BVA-01..12 | PASS |
| **§4.7** | **Principle 7: Absence of Errors Fallacy** | `tests/acceptance/test_uat_scenarios.sh` | Scientist/Student acceptance workflows | PRIN-07, AT-UAT-01..05 | PASS |
| **§5.1** | **White Box: Statement Coverage** | `Makefile` (`coverage`), `gcov` | Statement execution verification | WB-STMT-01..10 | PASS |
| **§5.2** | **White Box: Branch / Condition Coverage** | `tests/unit/test_wb_bigint.c`, `test_wb_fast_math.c` | Carry/borrow, Knuth D add-back, sign combos | WB-BR-01..15 | PASS |
| **§5.3** | **White Box: Path Coverage** | `tests/unit/test_wb_parser.c`, `test_wb_eval.c` | Independent paths in AST evaluation & parser | WB-PATH-01..20 | PASS |
| **§5.4** | **White Box: Loop Testing** | `tests/unit/test_wb_bigint.c`, `test_wb_functions.c` | Loops: 0, 1, 2, typical, max-1, max iterations | WB-LOOP-01..15 | PASS |
| **§6.1** | **Black Box: Equivalence Partitioning** | `tests/data/test_cases_equiv_partition.csv` | Valid/invalid classes for numbers, ops, parens | BB-EQ-01..20 | PASS |
| **§6.2** | **Black Box: Boundary Value Analysis** | `tests/data/test_cases_bva.csv` | Min-1, min, max, max+1 for precision, nesting, fact | BB-BVA-01..25 | PASS |
| **§6.3** | **Black Box: Decision Table Testing** | `tests/data/test_cases_decision_table.csv` | Operator x Operand Type matrix (Rulebook §12) | BB-DT-01..30 | PASS |
| **§6.4** | **Black Box: Error Guessing** | `tests/data/test_cases_error_guessing.csv` | 0/0, 0^-2, lone operators, trailing dot, illegal chars | BB-EG-01..30 | PASS |
| **§6.5** | **Black Box: State Transition Testing** | `tests/data/test_cases_state_trans.csv` | REPL state machine: Idle->Eval->Ans->Error->Idle | BB-ST-01..15 | PASS |
| **§6.6** | **Black Box: All-Pairs Testing** | `tests/blackbox/gen_pairwise.c` | Combinatorial pairs: Op x Left x Right x Sign x Mode | BB-PAIR-01..50 | PASS |
| **§7** | **Black Box vs White Box** | `docs/TEST_PLAN.md` §2, `docs/TRACEABILITY.md` | Dual perspective test suites | BBWB-01 | PASS |
| **§8** | **Unit Testing** | `tests/unit/test_wb_*.c` | One suite per module; public API invariant checks | UT-MOD-01..12 | PASS |
| **§9.1** | **Integration: Incremental Top-Down** | `tests/integration/test_it_topdown.c`, `tests/stubs/` | Top modules driving lower stubs | IT-TD-01..05 | PASS |
| **§9.2** | **Integration: Incremental Bottom-Up** | `tests/integration/test_it_bottomup.c`, `tests/drivers/` | Arithmetic primitives driving upper layers | IT-BU-01..05 | PASS |
| **§9.3** | **Integration: Non-Incremental Big Bang** | `tests/integration/test_it_bigbang.c` | Full module link smoke test | IT-BB-01 | PASS |
| **§9.4** | **Integration: Flow Testing** | `tests/integration/test_it_flow.c` | Session flow: expr -> token -> AST -> eval -> ans | IT-FLOW-01..04 | PASS |
| **§10.1** | **System: Performance Testing** | `tests/system/test_sys_perf.c` | Latency for 2^100000, 10000!, sqrt(2), div | ST-PERF-01..08 | PASS |
| **§10.2** | **System: Load & Concurrency Testing** | `tests/system/test_sys_load.sh`, `test_sys_concurrency.c` | 10k lines piped, POSIX multi-threaded contexts | ST-LOAD-01..04 | PASS |
| **§10.3** | **System: Usability Testing** | `tests/system/test_sys_usability.sh` | Help banner, prompt rendering, caret alignment | ST-USE-01..05 | PASS |
| **§10.4** | **System: Regression Testing** | `tests/system/test_sys_regression.sh` | Golden results comparison & defect regressions | ST-REG-01..05 | PASS |
| **§10.5** | **System: Migration Testing** | `tests/system/test_sys_migration.sh` | GCC vs Clang, -O0/-O2/-O3, locale independence | ST-MIG-01..04 | PASS |
| **§10.6** | **System: Functional & Missing Functions** | `tests/system/test_sys_functional.sh` | Master matrix + rejection of tan, asin, etc. | ST-FUNC-01..10 | PASS |
| **§10.7** | **System: Recovery Testing** | `tests/system/test_sys_recovery.sh` | Post-error recovery, EOF handling, ulimit low mem | ST-REC-01..05 | PASS |
| **§10.8** | **System: Stress Testing** | `tests/system/test_sys_stress_fuzz.c` | 129 parens, 100k digit literal, LCG byte fuzzer | ST-STR-01..08 | PASS |
| **§10.9** | **System: Software & Hardware Testing** | `tests/system/test_sys_compat.c` | Word sizes, endianness, zero external libs (ldd) | ST-COMPAT-01..05 | PASS |
| **§11.1** | **Acceptance: User Acceptance (UAT)** | `tests/acceptance/test_uat_scenarios.sh` | Scientist & engineering student scenarios | AT-UAT-01..06 | PASS |
| **§11.2** | **Acceptance: Business Acceptance (BAT)**| `tests/acceptance/test_bat_operational.sh` | Standalone zero-dep binary, CLI flag `-e`, status | AT-BAT-01..04 | PASS |
| **§11.3** | **Acceptance: Regulations Acceptance (RAT)**| `tests/acceptance/test_rat_compliance.sh` | ISO C11 strict conformance, POSIX.1-2008, license | AT-RAT-01..04 | PASS |
| **§12** | **Alpha vs Beta Testing** | `tests/acceptance/test_alpha.sh`, `test_beta.sh` | Internal full cycles vs clean user session replay | AT-ALPHA-01, AT-BETA-01 | PASS |
| **§13** | **Verification vs Validation** | `docs/verification_review.md`, `docs/TEST_PLAN.md` §5 | Static review (structs/limits) vs Dynamic runs | VERIF-01, VALID-01 | PASS |
| **§14** | **Defect Life Cycle** | `docs/DEFECT_LOG.md`, `tools/defect.sh` | 8-state lifecycle implementation | DEFECT-MGT-01 | PASS |
| **§15** | **Testing vs Debugging** | `docs/TEST_PLAN.md` §6, `docs/DEFECT_LOG.md` | Testing vs debugging comparison & defect hypotheses | TST-DBG-01 | PASS |
| **§16** | **Software Quality Metrics** | `docs/QUALITY_REPORT.md` | 8 Dimensions, 3 Factors, PUM, Survey | SQM-METRIC-01 | PASS |
| **§17** | **Software Quality Management** | `docs/QUALITY_REPORT.md` §5 | Quality Planning, Assurance, and Control | SQM-PLAN-01 | PASS |
| **§18** | **Quality Assurance vs Quality Control** | `docs/QUALITY_REPORT.md` §6 | QA vs QC activity mapping and distinction | QA-QC-01 | PASS |

---

## 2. Rulebook Section Traceability (rulebook.md)

| Rulebook Section | Specification Scope | Executable Test Suite / Case ID | Expected Output / Invariant |
| :---: | :--- | :--- | :--- |
| **§3.1** | Allowed Characters (`0-9`, `.`, `e/E`, `+ - * / % ^ ! ( ) ,`) | `BB-EQ-01..08`, `test_cases_equiv_partition.csv` | Tokens scanned cleanly without character errors |
| **§3.2** | Forbidden Characters (`[]{}&\|~<>@$#\;:'"`) | `BB-EG-01..12`, `test_cases_error_guessing.csv` | Rejects with `Error: Incorrect expression` |
| **§4.1** | Canonical Zero & Integer Signatures | `UT-BIGINT-01..03`, `test_wb_bigint.c` | `size = 1, digits[0] = 0, sign = 0` |
| **§4.2** | Division Auto-Promotion (Exact vs Remainder) | `BB-DT-01..05`, `test_cases_decision_table.csv` | Non-zero remainder integer division promotes to `BigDecimal` |
| **§4.3** | Complex Format ($a + bi$) | `UT-CPX-01..05`, `test_wb_complex.c` | Complex arithmetic: `i*i = -1`, conjugate division |
| **§4.4** | Scientific Notation Input | `BB-EQ-09..12`, `test_cases_rulebook.csv` (C01–C15) | Parses `1e5`, `2.5e-3`, `1E+9` accurately |
| **§5.1–5.4**| Identifiers & Constants (`pi`, `e`, `i`, `ans`) | `UT-SYMTAB-01..05`, `BB-ST-01..05` | Built-in constants resolved; undefined vars rejected |
| **§6** | Operator Precedence Ladder (Levels 1–6) | `test_cases_rulebook.csv` (A01–A18) | Standard precedence: `*` before `+`, right-associative `^` |
| **§7** | Unary Sign Binding Rules | `test_cases_rulebook.csv` (D01–D12) | Correct resolution of `+-5`, `--5`, `-3^2` |
| **§8** | Memory Limits & Resource Ceilings | `ST-STR-01..05`, `test_sys_stress_fuzz.c` | Deep nesting ($>128$) rejected; $100{,}000$-digit handled |
| **§9.1** | Juxtaposition & Implicit Multiplication Rejection| `test_cases_rulebook.csv` (E01–E07) | Rejects `2(3)`, `2pi`, `3i`, `(2)sin(pi)` |
| **§10** | Built-in Functions (`sqrt`, `sin`, `cos`, `exp`, `ln`, `abs`, `gcd`, `pow`, `mod`, `fact`) | `test_cases_rulebook.csv` (B01–B20) | Exact evaluation within precision tolerances |
| **§11** | Factorial Constraints ($0 \le n \le 10000$) | `BB-BVA-01..06`, `test_cases_bva.csv` | Rejects $n < 0$, non-integers, and $n > 10000$ |
| **§12** | Type Promotion Matrix (Int, Dec, Complex) | `BB-DT-06..25`, `test_cases_decision_table.csv` | $\text{INT} \to \text{DEC} \to \text{COMPLEX}$ promotion |
| **§13** | Expression Nesting & Depth Limit | `BB-BVA-07..10`, `test_cases_bva.csv` | Nesting $\le 128$ passes; 129 fails cleanly |
| **§14** | REPL Meta-Commands (`mode`, `precision`, `help`, `exit`) | `BB-ST-06..12`, `test_cases_state_trans.csv` | Angular mode toggled; precision dynamically set |
| **§15** | Master Matrix Combinations (IDs A01–J05) | `tests/data/test_cases_rulebook.csv` | All 80+ authoritative rows tested via automated runner |
| **§16** | Diagnostic Error Strings | `BB-EG-13..25`, `test_cases_error_guessing.csv` | Exact matching of error messages |

