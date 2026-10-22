# High-Level Design (HLD) & System Architecture Document

**Project Name:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Project Identifier:** Precision-Calculator  
**Version:** 2.0.0  
**Document Classification:** System Architecture  

---

## 1. System Context & Architecture Overview

The **Precision-Calculator** system is architected as a modular, layered computational engine implemented entirely in ISO C11. It operates primarily as a standalone native CLI/REPL binary and an embeddable static/dynamic library (`libprecisioncalc`). Additionally, the engine supports headless batch execution, file scripting, and integration into networked services (via an HTTP microservice wrapper).

### 1.1 C4 Context Diagram (Level 1)

```mermaid
C4Context
    title System Context Diagram: Precision-Calculator Ecosystem

    Person(user, "User / Scientist", "Interactively executes high-precision calculations or runs scripts.")
    System(calc, "Precision-Calculator Engine", "C11 native engine parsing infix expressions and executing arbitrary-precision math.")
    System_Ext(script, "Automated Scripts / CI", "Shell scripts, batch files, benchmark runners.")
    System_Ext(http_svc, "Microservice Daemon (Optional)", "REST / HTTP daemon exposing calculation engine over sockets.")

    Rel(user, calc, "Inputs expressions via REPL / CLI args")
    Rel(script, calc, "Pipes input batches via stdin / stdout")
    Rel(http_svc, calc, "Invokes native C API (eval_node / eval_context)")
    Rel(calc, user, "Outputs exact digits & elapsed latency (ms)")
```

### 1.2 Architectural Principles
1. **Zero External Runtime Dependencies:** The calculation engine depends strictly on standard POSIX/ISO C runtime libraries (`libc`, `libm`). No GNU GMP, OpenSSL, or external framework is linked.
2. **Layered Separation of Concerns:** Divided into three autonomous worklets:
   * **Worklet 1 (Frontend):** Lexing, parsing, Abstract Syntax Tree (AST) construction, and terminal I/O.
   * **Worklet 2 (Primitive Core):** Base-$10^9$ limb representation, sign-magnitude representation, addition, subtraction, and schoolbook multiplication.
   * **Worklet 3 (Advanced Core & Runtime):** Karatsuba multiplication, Knuth Algorithm D division, fixed-point decimal arithmetic, complex numbers, transcendental functions, and post-order AST execution.
3. **Decoupled Data Contracts:** Information passes through clear boundaries as defined types:  
   $$\text{Raw String} \xrightarrow{\text{Lexer}} \text{TokenList} \xrightarrow{\text{Parser}} \text{AST (ExprNode)} \xrightarrow{\text{Evaluator}} \text{Value} \xrightarrow{\text{Renderer}} \text{Display String}$$

---

## 2. Container & Component Decomposition (C4 Level 2 & 3)

```mermaid
graph TD
    subgraph UI ["User Interface Layer"]
        CLI["CLI Argument Handler\n(main.c)"]
        REPL["Interactive REPL Loop\n(Worklet1/repl.c)"]
    end

    subgraph Front ["Worklet 1: Frontend & Infrastructure"]
        VAL["Line Validator\n(lexer.c)"]
        LEX["Lexer / Tokenizer\n(lexer.c)"]
        PARSE["Recursive-Descent Parser\n(parser.c)"]
        AST["AST Node Lifecycle\n(ast.c)"]
        IO["9-Digit Chunk Serializer\n(bigint_io.c)"]
        TMR["Monotonic Timer\n(timer.c)"]
    end

    subgraph CoreW2 ["Worklet 2: Basic Arithmetic"]
        MEM["BigInteger Memory Lifecycle\n(bigint.c)"]
        NORM["Canonical Zero & Normalizer\n(bigint.c)"]
        ADDSUB["Carry/Borrow Add & Sub\n(bigint.c)"]
        SBMUL["Schoolbook Multiplier O(N^2)\n(bigint.c)"]
    end

    subgraph CoreW3 ["Worklet 3: Advanced Arithmetic & Evaluation"]
        KARA["Karatsuba Multiplier O(N^1.585)\n(fast_math.c)"]
        KNUTH["Knuth Algorithm D Division\n(fast_math.c)"]
        DEC["Fixed-Point BigDecimal Engine\n(decimal.c)"]
        CMPLX["Complex Number Engine\n(complex.c)"]
        TRANS["Transcendental Functions\n(functions.c)"]
        EVAL["Post-Order AST Evaluator\n(eval.c)"]
        SYMTAB["Symbol Table & Constant Preload\n(eval.c)"]
    end

    CLI --> EVAL
    REPL --> VAL
    VAL --> LEX
    LEX --> PARSE
    PARSE --> AST
    PARSE --> EVAL
    EVAL --> SYMTAB
    EVAL --> CoreW2
    EVAL --> CoreW3
    EVAL --> TMR
    EVAL --> IO
```

---

## 3. Technology Stack & Architectural Decisions (ADRs)

### ADR-001: Monolithic C Engine vs. Third-Party Math Libraries
* **Status:** Accepted.
* **Context:** Scientific software frequently relies on GNU GMP (`libgmp`) or MPFR for multi-precision calculations.
* **Decision:** Implement arbitrary-precision storage and all arithmetic algorithms from scratch in ISO C11.
* **Rationale:**
  * Complete control over memory layout, cache friendliness, and algorithm thresholds.
  * Zero external licensing encumbrances (LGPL issues avoided).
  * High portability across microcontrollers, embedded POSIX platforms, and bare-metal environments.

### ADR-002: Base-$10^9$ Multi-Limb Representation
* **Status:** Accepted.
* **Context:** Arbitrary-precision numbers can be represented in base-2 ($2^{32}$ / $2^{64}$), base-10 (BCD), or base-$10^9$.
* **Decision:** Represent big integers as an array of 32-bit limbs in base $B = 10^9$:
  $$\text{Value} = \text{sign} \times \sum_{k=0}^{N-1} \text{digits}[k] \times (10^9)^k$$
* **Rationale:**
  * **Overflow Prevention:** The maximum product of two base-$10^9$ limbs is $(10^9 - 1)^2 \approx 10^{18} < 2^{63}$, which fits safely into standard `uint64_t` accumulator variables without assembly-level carry flags.
  * **Fast I/O Conversion:** Packing and unpacking decimal characters involves only 9-digit chunk operations rather than expensive repeated binary division by 10.

### ADR-003: Tagged Union `Value` Hierarchy with Automatic Promotion
* **Status:** Accepted.
* **Context:** The calculator must support integers, fractional numbers, and imaginary values seamlessly.
* **Decision:** Encapsulate results in a tagged union `Value` supporting `VAL_INT`, `VAL_DEC`, and `VAL_COMPLEX`. Mixed-type operations automatically promote upwards:
  $$\text{BigInteger (VAL\_INT)} \longrightarrow \text{BigDecimal (VAL\_DEC)} \longrightarrow \text{Complex (VAL\_COMPLEX)}$$
* **Rationale:**
  * Prevents premature conversion of pure integers to floating-point approximations.
  * Allows exact results ($10! = 3628800$) to remain integers while fractional results ($10/4 = 2.5$) promote automatically to `BigDecimal`.

### ADR-004: Recursive-Descent AST vs. Shunting-Yard Queue
* **Status:** Accepted.
* **Context:** Infix expression evaluation can be performed using Dijkstra's Shunting-Yard algorithm (direct RPN stack) or a recursive-descent parser producing an AST.
* **Decision:** Build a full Abstract Syntax Tree (`ExprNode`) using recursive descent.
* **Rationale:**
  * Clean support for right-associative power operators ($2^{3^2}$).
  * Support for variable arity function calls (`gcd(a, b)`, `sin(x)`).
  * Accurate source code column tracking for syntax error carets (`^`).
  * Extensible to multi-statement execution, optimizations, and constant folding.

---

## 4. Pipeline Data Flow & State Machine

```mermaid
stateDiagram-v2
    [*] --> Idle: Launch Engine
    Idle --> ReadInput: Prompt User / Read Args
    ReadInput --> ValidateLine: Input Received
    ValidateLine --> ReportSyntaxError: Invalid Chars / Unbalanced Parentheses
    ValidateLine --> Tokenize: Parentheses Balanced
    Tokenize --> ParseAST: TokenList Generated
    ParseAST --> ReportParseError: Grammar Violation
    ParseAST --> InitEvalContext: AST Root Valid
    InitEvalContext --> StartTimer: Attach Symbol Table
    StartTimer --> EvaluateTree: Post-Order Walk (eval_node)
    EvaluateTree --> CheckError: Traversal Complete
    CheckError --> ReportRuntimeError: DivByZero / Domain / StackLimit
    CheckError --> StopTimer: Success
    StopTimer --> FormatString: value_to_string()
    FormatString --> DisplayOutput: Print Value + Elapsed Latency
    DisplayOutput --> UpdateAns: Store in symtab["ans"]
    UpdateAns --> CleanupMemory: Free AST & Tokens
    CleanupMemory --> Idle: Ready for Next Expression
    ReportSyntaxError --> Idle
    ReportParseError --> Idle
    ReportRuntimeError --> Idle
```

---

## 5. Subsystem Interaction Flows

### 5.1 Arithmetic Computation Lifecycle
```mermaid
sequenceDiagram
    autonumber
    actor User
    participant CLI as main.c / repl.c
    participant Parser as parser.c
    participant Eval as eval.c
    participant Arith as fast_math.c / bigint.c
    participant Out as bigint_io.c

    User->>CLI: Enter "2^100 + 5"
    CLI->>Parser: parse_expression("2^100 + 5")
    Parser-->>CLI: ExprNode(OP_ADD, bin)
    CLI->>Eval: eval_node(root, ctx)
    Eval->>Eval: eval_node(left = 2^100)
    Eval->>Arith: bigint_pow(base=2, exp=100)
    Arith-->>Eval: BigInteger(1267650600228229401496703205376)
    Eval->>Eval: eval_node(right = 5)
    Eval->>Arith: bigint_add(left, right)
    Arith-->>Eval: BigInteger(1267650600228229401496703205381)
    Eval-->>CLI: Value(VAL_INT)
    CLI->>Out: bigint_to_string(result)
    Out-->>CLI: "1267650600228229401496703205381"
    CLI->>User: Display Result & Latency (ms)
```

---

## 6. Service Topology & Microservice Integration (Extension)

For cloud or networked microservice deployment, the native engine is exposed via a lightweight HTTP daemon wrapper:

```mermaid
graph LR
    subgraph Clients ["Client Applications"]
        Browser["Web Frontend (React / WASM)"]
        Mobile["Mobile Apps"]
        Worker["Data Science Pipeline (Python / R)"]
    end

    subgraph Service ["Microservice Container"]
        Gateway["Reverse Proxy (Nginx)"]
        Daemon["HTTP API Daemon (libmicrohttpd / Go wrapper)"]
        Engine["libprecisioncalc (C Native Engine)"]
    end

    Browser -->|HTTP POST /api/v1/eval| Gateway
    Mobile -->|HTTP POST /api/v1/eval| Gateway
    Worker -->|REST API| Gateway
    Gateway --> Daemon
    Daemon -->|In-Process FFI C Call| Engine
```

* **Process Concurrency:** Thread-safe execution is achieved by providing each thread or worker process with its own allocated `EvalContext` and isolated `SymbolTable`.
* **State Management:** Stateless across REST requests; optional session cookies map to server-side cached symbol tables.
* **Failure Isolation:** Worker processes running evaluations operate under CPU timeout alarms (`setitimer`) and virtual memory limits (`RLIMIT_AS`) to guard against malicious recursive expressions.
