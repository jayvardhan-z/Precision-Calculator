# Developer Setup & Repository Guide

**Project Name:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Project Identifier:** Precision-Calculator  
**Version:** 2.0.0  
**Document Classification:** Developer Onboarding & Engineering Handbook  

---

## 1. Quick Start

### 1.1 Prerequisites & Toolchain Verification
The engine requires an ISO C11 compliant compiler and standard POSIX build utilities.

| Dependency | Minimum Version | Verified Versions | Purpose |
| :--- | :--- | :--- | :--- |
| **GCC** or **Clang** | GCC 7.0+ / Clang 6.0+ | Apple Clang 15+, GCC 11.4+ | C11 compilation & static analysis |
| **GNU Make** | 3.81+ | GNU Make 3.81 / 4.3 | Automated incremental build system |
| **Git** | 2.20+ | Git 2.39+ | Version control |
| **Valgrind** *(Linux)* | 3.15+ | Valgrind 3.19+ | Heap profiling & leak detection |
| **LLDB / GDB** | Any modern | LLDB 15+ / GDB 12+ | Interactive debugging |

Check your installed toolchain:
```bash
cc --version
make --version
git --version
```

---

## 2. Repository Structure

```
Precision-Calculator/
├── Makefile                    # Modular build specification with automatic dependency tracking
├── README.md                   # High-level project summary and worklet plan
├── docs/                       # Formal engineering specifications & runbooks
│   ├── SRS_PRD.md              # Requirements & User Stories
│   ├── HLD_System_Architecture.md # System Architecture & C4 Diagrams
│   ├── LLD_Technical_Design.md # Low-Level Specs & Algorithm Pseudocode
│   ├── API_Interface_Contract.md # C API & OpenAPI 3.1 REST Specification
│   ├── Developer_Setup_Guide.md # This guide
│   ├── Test_Plan_Acceptance_Cases.md # Acceptance tests & verification
│   └── Deployment_Runbook.md   # Deployment, packaging & operations
├── main.c                      # Application entry point & CLI dispatch
├── Worklet1/                   # Frontend, Lexer, Parser, AST & I/O
│   ├── ast.h / ast.c           # AST node representations & recursive deallocator
│   ├── bigint_io.h / .c        # Base-10^9 limb packing/unpacking & line reading
│   ├── lexer.h / lexer.c       # Character validator & token scanner
│   ├── parser.h / parser.c     # Recursive-descent precedence parser
│   ├── repl.h / repl.c         # Interactive REPL loop & command handler
│   └── timer.h / timer.c       # Monotonic clock interval timing
├── Worklet2/                   # Arbitrary-Precision Core Storage & Basic Arithmetic
│   └── bigint.h / bigint.c     # Limbs, canonical zero, addition, subtraction, schoolbook mul
├── Worklet3/                   # Advanced Math, Decimals, Complex & Evaluator
│   ├── fast_math.h / .c        # Karatsuba multiplication & Knuth Algorithm D division
│   ├── decimal.h / decimal.c   # Fixed-point BigDecimal engine & rounding
│   ├── complex.h / complex.c   # Complex number arithmetic & conjugate division
│   ├── functions.h / .c        # Transcendental functions (sqrt, exp, ln, sin, cos)
│   └── eval.h / eval.c         # AST evaluator, type promotion, symbol table
└── obj/                        # Automatically generated compilation artifacts (.o, .d)
```

---

## 3. Build & Execution Workflow

### 3.1 Standard Optimized Production Build
Compiles all worklets with `-O2` optimization and strict C11 compliance flags:
```bash
# Clean previous artifacts and build binary
make clean && make
```
* **Output Binary:** `./calculator`

### 3.2 Running the Application

#### Interactive REPL Mode
```bash
./calculator
```
Output:
```text
Arbitrary-Precision Scientific Calculator
Type ':help' for meta-commands or ':exit' to quit.

calc> x = 2^64
  = 18446744073709551616  [0.0050 ms]

calc> x * 2
  = 36893488147419103232  [0.0040 ms]

calc> sin(pi / 4)
  = 0.70710678118654752440  [0.0460 ms]

calc> :exit
```

#### Batch CLI Mode
```bash
./calculator "2 + 3 * 4"
# Output:
#   = 14  [0.0040 ms]

./calculator "(2 + 3*i) * (4 + 5*i)"
# Output:
#   = -7 + 22i  [0.0170 ms]
```

#### Pipeline / Stdin Redirection Mode
```bash
echo "sqrt(144) + 10" | ./calculator
```

---

## 4. Debugging & Quality Assurance

### 4.1 AddressSanitizer & UndefinedBehaviorSanitizer Build
To compile with memory-sanitizers to detect out-of-bounds access or undefined behavior:
```bash
make clean
make CFLAGS="-IWorklet1 -IWorklet2 -IWorklet3 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -pedantic -std=c11 -g -fsanitize=address,undefined" LDFLAGS="-fsanitize=address,undefined -lm"
```
Run any test expression:
```bash
./calculator "2^100"
```

### 4.2 Valgrind Memory Leak Audit (Linux)
Validate that all limbs, strings, and AST nodes are deallocated without memory leaks:
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --error-exitcode=1 \
         ./calculator "sin(pi / 6) + exp(2)"
```
* **Acceptable Result:** `All heap blocks were freed -- no leaks are possible (ERROR SUMMARY: 0 errors)`.

### 4.3 Interactive Debugging (GDB / LLDB)
Launch with LLDB on macOS:
```bash
lldb ./calculator
(lldb) breakpoint set --name eval_binary
(lldb) run "10 + 20"
(lldb) print l.as.i
(lldb) continue
```

---

## 5. Coding Standards & Conventions

All contributors must adhere to the following conventions:

1. **Language Standard:** Strict ISO C11 (`-std=c11`). No compiler-specific extensions (avoid `__attribute__`, non-standard GCC builtins without feature guards).
2. **Comment Style:** **Always use `//` single-line comments**, even for multi-line explanations. Document internal loop invariants, branch decisions, and algorithmic steps inside function bodies.
3. **Compiler Warnings:** Code must compile with **zero warnings** under `-Wall -Wextra -pedantic`. Unused variables must be cast with `(void)var;` if intentionally unreferenced.
4. **Memory Ownership:**
   * Every `*_init()` must have a matching `*_free()`.
   * Never store pointers into temporary strings; use `strdup()` and document ownership.
   * Functions returning heap allocations must state: `Caller must free the returned string with free()`.
5. **Base-$10^9$ Invariant:** Limbs must never exceed $999{,}999{,}999$. When normalizing a BigInteger, leading zero limbs must be stripped, and zero must have `sign = 0` and `size = 0`.
