# Precision-Calculator: Software Maintenance & Evolution Plan

## 1. Introduction & Objectives
This Maintenance Plan defines the engineering governance, change management procedures, and ongoing support workflows for **Precision-Calculator** across its operational lifecycle. It adheres to ISO/IEC 14764 (Software Engineering — Software Life Cycle Processes — Maintenance) and IEEE 1219 standards.

---

## 2. The Four Maintenance Categories

```
                    ┌───────────────────────────────────────────────┐
                    │       PRECISION-CALCULATOR MAINTENANCE        │
                    └───────────────────────┬───────────────────────┘
                                            │
        ┌───────────────────┬───────────────┴───────────────┬───────────────────┐
        ▼                   ▼                               ▼                   ▼
┌───────────────┐   ┌───────────────┐               ┌───────────────┐   ┌───────────────┐
│  Corrective   │   │   Adaptive    │               │  Perfective   │   │  Preventive   │
│ Maintenance   │   │  Maintenance  │               │  Maintenance  │   │  Maintenance  │
└───────┬───────┘   └───────┬───────┘               └───────┬───────┘   └───────┬───────┘
        │                   │                               │                   │
  Fix BUG-01/02       Port to 32-bit                  SIMD Limb Ops       Refactor eval.c
  Reciprocal Zero     RISC-V / ARM64                  Montgomery Red      Memory hardening
```

### 2.1. Corrective Maintenance (Bug Fixing)
- **Scope:** Rectifying discovered defects that deviate from the specification oracle (`rulebook.md`).
- **Immediate Work Queue:**
  1. **Resolve BUG-01 (`Worklet3/eval.c`):** Fix `0 ^ -2` reciprocal division path to call `set_error(ctx, ERR_DIV_BY_ZERO, pos, "Division by zero")`.
  2. **Resolve BUG-02 (`Worklet1/repl.c`):** Update `run_expression("")` to check for empty or whitespace-only strings before entering execution loop and return exit code 1 with `Error: Incorrect expression`.
- **SLA:**
  - Critical/Major defects: Patched and verified within 24 hours.
  - Minor defects: Patched in scheduled maintenance sprint (bi-weekly).

### 2.2. Adaptive Maintenance (Platform & Environment Evolution)
- **Scope:** Ensuring the software continues to function reliably as host operating systems, toolchains, and hardware architectures evolve.
- **Planned Adaptations:**
  1. **32-Bit Word Architecture Compatibility:** While `uint64_t` accumulator logic is verified, add explicit regression builds under 32-bit ARM/i386 toolchains (`-m32`).
  2. **Compiler Modernization:** Continuous verification against new major releases of Clang (v18+) and GCC (v14+).
  3. **Embedded POSIX Targets:** Adaptation for embedded Linux and FreeBSD micro-kernels.

### 2.3. Perfective Maintenance (Performance & Feature Enhancement)
- **Scope:** Improving performance and computational efficiency based on user operational feedback.
- **Planned Enhancements:**
  1. **SIMD Vectorization:** Optimize multi-limb BigInteger addition and schoolbook multiplication using ARM Neon and x86 AVX2 vector intrinsics.
  2. **Crossover Threshold Autotuning:** Dynamic runtime detection of CPU cache sizes to adjust `KARATSUBA_CROSSOVER` (currently fixed at 16 limbs).
  3. **Montgomery Multiplier:** Implement Montgomery reduction for modular exponentiation (`bigint_mod_pow`).

### 2.4. Preventive Maintenance (Software Re-engineering & Refactoring)
- **Scope:** Proactively identifying and eliminating latent software decay, code smells, and technical debt before failures manifest.
- **Planned Activities:**
  1. **Cyclomatic Complexity Reduction:** Decompose `eval_node()` in `Worklet3/eval.c` (currently 550 lines) into dedicated sub-evaluators (`eval_unary_node`, `eval_binary_node`, `eval_call_node`).
  2. **Static Analysis Audits:** Scheduled static analysis via Clang Static Analyzer, `scan-build`, and Splint.
  3. **Memory Bounds Hardening:** Formal verification that limb array capacity resizing never triggers signed integer wrapping.

---

## 3. Maintenance Change Request (CR) Workflow

```
   User / QA Report
          │
          ▼
   ┌──────────────┐
   │ Triage & CR  │ ──── Rejected ────► [Closed: Not a Bug]
   │ Assignment   │
   └──────┬───────┘
          │ Approved
          ▼
   ┌──────────────┐
   │ Impact       │ (Examine AST, BigInt, or Parser dependencies)
   │ Analysis     │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Branch & Fix │ (Git branch: fix/<bug-id> or feat/<cr-id>)
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Mandatory    │ ───► make test
   │ Verification │ ───► make sanitize (Zero leaks / UB)
   │ Pipeline     │ ───► make coverage (No regression in %)
   └──────┬───────┘
          │ PASS
          ▼
   ┌──────────────┐
   │ Code Review  │ ────► Merge to main & Tag Release (SemVer)
   └──────────────┘
```

---

## 4. Defect State Machine Governance (`tools/defect.sh`)

Every maintenance engineer must track defect transitions using the automated defect state machine tool:

```bash
# Workflow example:
./tools/defect.sh new "Reciprocal division error" "Worklet3/eval.c" "Major" "0 ^ -2 emits empty Error:"
./tools/defect.sh assign BUG-01 "systems-eng-1"
./tools/defect.sh status BUG-01 OPEN
./tools/defect.sh fix BUG-01 "Worklet3/eval.c:575 added set_error call"
./tools/defect.sh retest BUG-01
./tools/defect.sh verify BUG-01 "make test passed 100%"
./tools/defect.sh close BUG-01
```

---

## 5. Regression Testing & Release Policy

1. **Zero Production Code Changes without Tests:**
   Every code change must include at least one corresponding unit test in `tests/unit/` or regression test in `tests/system/test_sys_regression.sh`.
2. **Mandatory Pipeline Gate:**
   No pull request or commit may be merged unless `make test`, `make sanitize`, and `make coverage` exit with return code 0.
3. **Semantic Versioning (SemVer 2.0.0):**
   - **PATCH (`x.y.Z`):** Backward-compatible corrective bug fixes (e.g. `1.0.1` for BUG-01 and BUG-02).
   - **MINOR (`x.Y.0`):** New mathematical functions or CLI flags added in a backward-compatible manner.
   - **MAJOR (`X.0.0`):** Incompatible API changes to `eval.h` or `bigint.h` structs.

---

## 6. Hotfix & Rollback Procedures

If a critical flaw is detected in production:
1. **Rollback:** Immediately revert the deployed binary to the last stable Git release tag (e.g., `git checkout v1.0.0`).
2. **Hotfix Isolation:** Branch directly from the release tag (`hotfix/critical-fix`), implement targeted fix, verify with `make sanitize`, and tag patch release (`v1.0.1`).
3. **Post-Mortem Review:** Document root cause, detection gap, and preventive actions within 48 hours.

