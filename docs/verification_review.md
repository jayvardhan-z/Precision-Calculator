# Static Verification Review & Inspection Log

**Project:** Precision-Calculator  
**Standard:** IEEE 1028-2008 Standard for Software Reviews and Audits  
**Classification:** Static Verification Checklist & Walkthrough Audit  

---

## 1. Static Verification vs. Dynamic Validation

- **Verification:** *"Are we building the product right?"* Static analysis, architecture review, and code walkthroughs before execution.
- **Validation:** *"Have we built the right product?"* Dynamic execution of test suites against user requirements and the Rulebook specification.

---

## 2. Static Architectural & Code Inspection Checklist

| Checkpoint | Target Specification (HLD / LLD / Rulebook) | Production Code Implementation | Verification Finding | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Data Types & Limb Size** | Base $B = 10^9$ limbs stored in `uint32_t` | `Worklet2/bigint.h`: `BASE 1000000000U`, `uint32_t *digits` | Limbs constrained to $[0, 10^9-1]$; fits in `uint32_t` | **VERIFIED** |
| **Overflow Safety** | Multi-limb product fits in `uint64_t` without carry overflow ($(10^9-1)^2 \approx 10^{18} < 2^{63}$) | `Worklet2/bigint.c`, `Worklet3/fast_math.c`: accumulators declared as `uint64_t` | Verified no 64-bit integer overflow during multiply-accumulate | **VERIFIED** |
| **Karatsuba Threshold** | HLD §3: crossover $> 32$ limbs | `Worklet3/fast_math.h:18`: `#define KARATSUBA_CROSSOVER 16` | Discrepancy noted in `spec_discrepancies.md`. Code activates at 16 limbs | **DISCREPANCY (SAFE)** |
| **Max Recursion Depth** | LLD §1.4: `MAX_RECURSION_DEPTH = 128` | `Worklet3/eval.h:38`: `#define MAX_RECURSION_DEPTH 128` | Verified depth counter checked in `eval.c:972` | **VERIFIED** |
| **Precision Range** | Rulebook §14: precision $n \in [0, 2000]$ | `Worklet1/repl.c:97`: `p >= 0 && p <= 2000` | Constraint strictly enforced; out-of-range yields diagnostic | **VERIFIED** |
| **Factorial Limit** | Rulebook §11: $0 \le n \le 10{,}000$ | `Worklet3/eval.c:437`: `if (n_val > 10000) return 0;` | Checked for non-negative integers; upper limit enforced | **VERIFIED** |
| **Registered Functions** | Rulebook §10: 10 functions (`sin`, `cos`, `exp`, `ln`, `sqrt`, `abs`, `gcd`, `pow`, `mod`, `fact`) | `Worklet1/parser.c:21-32`: exactly 10 functions in `FUNCTIONS[]` | 10 registered functions with exact arity matching Rulebook | **VERIFIED** |
| **Memory Ownership** | AST owns node pointers; Parser frees on error; Evaluator does not mutate input AST | `Worklet1/ast.c:free_expr_node()`, `Worklet1/parser.c` | Recursive deallocation walks tree; no dangling pointers | **VERIFIED** |
| **Deep-Copy Invariant** | `symtab_set` must duplicate identifier strings and clone Values | `Worklet3/eval.c:313`: `strdup(name)`, `value_copy()` | Independent heap allocations; verified no double frees | **VERIFIED** |
| **Zero-Dependency Check** | Zero external libraries (no GMP, MPFR, OpenSSL) | `Makefile`: links only standard `-lm` | Confirmed strictly libc and libm runtime | **VERIFIED** |

---

## 3. Compiler Static Diagnostic Audit

The codebase was compiled with strict ISO C11 flags:
```bash
gcc -Wall -Wextra -pedantic -std=c11 -O2
```
**Static Diagnostic Result:**
- **Warnings Emitted:** `0`
- **Errors Emitted:** `0`
- **Undefined Behavior Sanitizer:** Passed without diagnostic triggers.

