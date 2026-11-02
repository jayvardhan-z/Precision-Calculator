# Formal Defect & Bug Tracking Log

**Project:** Precision-Calculator  
**Standard:** IEEE 1044-2009 Classification for Software Anomalies  
**Document Identifier:** DOC-BUG-2026-01  
**Classification:** Quality Assurance Defect Repository  

---

## 1. Defect Life Cycle Architecture

Every defect detected during automated testing or manual inspection progresses through the formal state machine below:

```
Defect State Machine:
  [NEW] ──► [ASSIGNED] ──► [OPEN] ──┬──► [DUPLICATE]
                                    ├──► [REJECTED]
                                    ├──► [DEFERRED]
                                    └──► [FIXED] ──► [RETEST] ──┬──► [VERIFIED] ──► [CLOSED]
                                                                 └──► [REOPENED] ──► [OPEN]
```

### State Definitions
1. **`NEW`**: Detected by automated harness or manual tester; reproduction steps recorded.
2. **`ASSIGNED`**: Assigned to the responsible module author (Worklet 1, 2, or 3).
3. **`OPEN`**: Developer actively investigating root cause and formulating a fix.
4. **`DUPLICATE`**: An identical defect has already been reported.
5. **`REJECTED`**: Not a genuine defect; behavior matches the authoritative Rulebook specification.
6. **`DEFERRED`**: Valid defect of lower severity postponed to a subsequent release milestone.
7. **`FIXED`**: Code fix implemented in developer branch with associated regression test.
8. **`RETEST`**: Testing team independently re-executes reproduction steps on the new build.
9. **`REOPENED`**: Defect persists or regressed during retest.
10. **`VERIFIED`**: Fix confirmed; all regression suites pass.
11. **`CLOSED`**: Defect permanently archived.

---

## 2. Defect Summary & Pareto 80/20 Clustering Analysis

### Module Defect Distribution

| Module | Worklet | Total Defects Detected | Critical | High | Medium | Low | % of Total Defects |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| `fast_math.c` (Knuth D / Karatsuba) | W3 | 0 | 0 | 0 | 0 | 0 | 0% |
| `functions.c` (Transcendental Series)| W3 | 0 | 0 | 0 | 0 | 0 | 0% |
| `decimal.c` (Scale Alignment)        | W3 | 0 | 0 | 0 | 0 | 0 | 0% |
| `bigint.c` (Limb Arithmetic)         | W2 | 0 | 0 | 0 | 0 | 0 | 0% |
| `lexer.c` / `parser.c` (Grammar)     | W1 | 0 | 0 | 0 | 0 | 0 | 0% |
| `eval.c` (AST Traversal)             | W3 | 0 | 0 | 0 | 0 | 0 | 0% |
| `repl.c` / `main.c` (CLI Interface)  | W1 | 0 | 0 | 0 | 0 | 0 | 0% |
| **TOTAL** | — | **0** | **0** | **0** | **0** | **0** | **100%** |

*(Note: Table is automatically updated dynamically after test suite execution. Discrepancies and failures during test execution are recorded below without modifying production source code.)*

---

## 3. Active & Historical Defect Records

*(Defects detected during test execution will be appended here via `tools/defect.sh`)*

### Defect: BUG-01 - Zero base with negative exponent produces empty error message
- **Module:** eval.c
- **Severity:** High | **Priority:** P1
- **Found By:** csv_runner | **Date:** 2026-11-02 22:24
- **Current Status:** `NEW`
- **Expected Result:** Error: Division by zero
- **Actual Result:** Error:
- **Reproduction Steps:** `./calculator '0 ^ -2'`
- **History:**
  - [2026-11-02 22:24] Logged as `NEW` by csv_runner.

### Defect: BUG-02 - Empty and whitespace expressions exit silently instead of Error: Incorrect expression
- **Module:** repl.c
- **Severity:** Medium | **Priority:** P2
- **Found By:** csv_runner | **Date:** 2026-11-02 22:24
- **Current Status:** `NEW`
- **Expected Result:** Error: Incorrect expression
- **Actual Result:** empty stdout
- **Reproduction Steps:** `./calculator ''`
- **History:**
  - [2026-11-02 22:24] Logged as `NEW` by csv_runner.
