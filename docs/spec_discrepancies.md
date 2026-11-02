# Specification Discrepancies & Ambiguities Log

**Project:** Precision-Calculator  
**Standard Reference Documents:**
- High-Level Design (`HLD_System_Architecture.md`)
- Low-Level Design (`LLD_Technical_Design.md`)
- Input & Grammar Rulebook (`rulebook.md`)
- Software Requirements Specification (`SRS_PRD.md`)

This document records formal discrepancies, contradictions, and ambiguities identified across the architectural, design, and rulebook specifications during test suite engineering.

---

## 1. Discrepancy Matrix

| ID | Topic / Item | Rulebook Specification | LLD / HLD Specification | Production Code Behavior | Resolution / Oracle Decision |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **SPEC-DISC-01** | **Canonical Zero Representation** | **Rulebook §4.1:**<br>`sign = 0, size = 1, digits[0] = 0`<br>"Zero has exactly 1 limb whose value is 0" | **LLD §1.1 & §3.1:**<br>`sign = 0, size = 0`<br>"Normalized zero invariant is strictly size = 0" | `bigint.c`: `bigint_normalize()` sets `a->size = 1, a->digits[0] = 0, a->sign = 0` when value is zero. | **Rulebook wins for public behavior.** The test suite verifies `size = 1, digits[0] = 0, sign = 0` as the normalized zero invariant per Rulebook §4.1 and code implementation. |
| **SPEC-DISC-02** | **Function Arity Error Reporting** | **Rulebook §10 & Master Table:**<br>Reports arity mismatches as general syntax failure: `Error: Incorrect expression` | **LLD §1.4 Error Table:**<br>Specifies dedicated error code `ERR_INVALID_ARITY` (code 4) | In `parser.c`, arity mismatch triggers parser error and REPL prints `Error: Incorrect expression`. `eval.c` has `ERR_INVALID_ARITY` in enum but parser catches it earlier. | **Black-box oracle expects Rulebook string:** `Error: Incorrect expression`. Unit/White-box tests check `ERR_INVALID_ARITY` enum validity. |
| **SPEC-DISC-03** | **Variable Storage Feature** | **Older Docs (SRS, API):**<br>Mentioned variable assignment (`x = expr`) and `vars` command. | **Updated Rulebook & User Requirement:**<br>Variable storage feature explicitly disabled. Only pure expressions, mathematical constants (`pi`, `e`, `i`), and `ans` are allowed. | `repl.c` rejects assignment syntax as `Error: Incorrect expression` and does not provide `vars` command. | **Current requirement:** Variable storage is disabled. Tests verify rejection of `<var> = <expr>` and absence of `vars` command. |
| **SPEC-DISC-04** | **Trigonometric Evaluation on Exact Boundaries** | **Rulebook §10:**<br>`sin(pi)` $= 0.0$, `cos(pi/2)` $= 0.0$ | **LLD §2.5:**<br>Computes series truncated to target precision + 8 guard digits. | Finite Taylor series approximation yields values with trailing residual digits around $10^{-p}$ unless rounded. | **Oracle checks tolerance:** Black-box runner trims/compares up to evaluated precision (20 decimal places). |
| **SPEC-DISC-05** | **Undefined Variable Error Diagnostic** | **Rulebook §5.4 & LLD §1.4:**<br>`Error: Undefined variable '<name>'` | **Rulebook §16 Table:**<br>Lists `ERR_UNDEFINED_VAR` mapped to `Error: Undefined variable '<name>'` | `eval.c:999` emits `Undefined variable '%s'`. | **Consistent:** Tests assert `Error: Undefined variable '<name>'`. |
| **SPEC-DISC-06** | **Scientific Notation Exponent Case Sensitivity** | **Rulebook §3.1:**<br>Permits both `e` and `E`. | **LLD Lexer spec:**<br>`e` or `E` adjacent to digits. | `lexer.c:228` accepts both `*p == 'e' || *p == 'E'`. | **Consistent:** Tests cover both uppercase and lowercase exponent markers. |

---

## 2. Testing Implications & Strategy
1. **Rulebook as Primary Oracle:** Whenever a mismatch exists between documentation and user-facing behavior, the Rulebook serves as the authoritative black-box oracle.
2. **Defect vs Discrepancy:** If the production code deviates from the Rulebook master matrix (A01–J05), a defect is logged in `docs/DEFECT_LOG.md` without modifying production code.
3. **Traceability:** All tests affected by these discrepancies explicitly reference the relevant `SPEC-DISC` entry.

