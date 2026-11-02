# Precision-Calculator Input & Grammar Rulebook

**Project:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Version:** 2.0.0  
**Target Environment:** ANSI C (C11) / POSIX.1-2008  
**Scope:** Complete Specification of Valid vs. Invalid Inputs, Operators, Combinations, and Types

---

## 1. Introduction & Executive Summary

The **Arbitrary-Precision Calculator** is an industrial-grade numerical engine implemented from first principles in ISO C11. It evaluates arbitrary-length numbers and mathematical expressions without machine-word truncation (no IEEE 754 precision loss) and without third-party dependencies (zero GMP/MPFR dependencies).

To achieve mathematical rigor and predictable evaluation, the engine enforces:
1. **Strict Operator Grammar:** Recursive-descent AST parsing with 6 formal precedence levels.
2. **Explicit Multiplication Requirement:** **Implicit multiplication is strictly prohibited** (e.g., `2(3)` and `2x` are rejected).
3. **Dynamic Type Promotion:** Seamless widening along $\text{BigInteger} \longrightarrow \text{BigDecimal} \longrightarrow \text{Complex}$.
4. **Defensive Arithmetic:** Explicit runtime domain checking (zero-division guards, recursion depth limits, factorial bounds).

This Rulebook serves as the definitive reference for what constitutes **valid** and **invalid** input across all syntax categories and combinations.

---

## 2. Invocation & Execution Modes

The calculator can be invoked in four distinct execution modes:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Execution Entry Points                          │
├──────────────────┬──────────────────┬─────────────────┬────────────────┤
│ Interactive REPL │ Single Quoted    │ Flag Invocation │ Shell Token    │
│ ./calculator     │ ./calculator     │ ./calculator -e │ ./calculator   │
│                  │   "2 + 3 * 4"    │   "100! / 99!"  │   2 + 3 * 4    │
└──────────────────┴──────────────────┴─────────────────┴────────────────┘
```

| Mode | Syntax Example | Valid Inputs | Behavior / Output |
| :--- | :--- | :--- | :--- |
| **Interactive REPL** | `./calculator` | Expressions, assignments (`x = 5`), meta-commands (`mode`, `precision`, `vars`, `help`, `exit`) | Displays persistent `> ` prompt, result, and elapsed time in milliseconds. Variables persist across lines. |
| **Direct CLI String** | `./calculator "2^10 + 5"` | Any valid single expression | Evaluates expression, prints result with timing, returns exit code `0`. |
| **Flag Execution** | `./calculator -e "sqrt(144)"` | `-e` followed by expression | Equivalent to direct CLI string. Useful in scripts. |
| **Multi-Argument CLI** | `./calculator 2 + 3 '*' 4` | Unquoted CLI tokens | Concatenates arguments with spaces before evaluation. **Warning:** Shell metacharacters (`*`, `(`, `)`, `^`, `!`) must be escaped or quoted from the shell. |
| **Help Flag** | `./calculator -h` or `--help` | `-h` or `--help` | Prints usage manual and exits with code `0`. |

---

## 3. Lexical Rules & Permitted Character Sets

### 3.1 Allowed Characters

Every character in an expression must belong to one of the following recognized lexical categories:

| Category | Permitted Characters | Role & Examples |
| :--- | :--- | :--- |
| **Digits** | `0 1 2 3 4 5 6 7 8 9` | Numeric literals: `0`, `42`, `1000000000000000000000000000000` |
| **Decimal Point** | `.` | Fractional numbers: `3.14159`, `.5`, `0.001` (Must be adjacent to at least one digit) |
| **Exponent Marker** | `e`, `E` | Scientific notation: `1e5`, `2.5e-3`, `1E+9` |
| **Identifier Letters** | `a-z`, `A-Z`, `_` | Function names (`sin`, `gcd`), variable names (`x`, `total_sum`), constants (`pi`, `e`, `i`, `ans`) |
| **Arithmetic Operators** | `+`, `-`, `*`, `/`, `%`, `^`, `!` | Addition, subtraction/negation, multiplication, division, modulo, exponentiation, factorial |
| **Grouping Parentheses**| `(`, `)` | Sub-expression evaluation order and function call parameter wrapping |
| **Argument Separator** | `,` (comma) | Separates parameters in multi-argument functions: `gcd(48, 18)`, `mod(23, 5)` |
| **Assignment Operator** | `=` | REPL variable definition only: `radius = 15` (Forbidden inside pure expressions) |
| **Whitespace** | Space (` `), Tab (`\t`), Newline (`\n`) | Completely ignored between tokens; used for readability |

### 3.2 Forbidden Characters (Instant Syntax Error)

The following characters are **illegal** anywhere in an expression:

| Character Class | Forbidden Characters | Example Invalid Input | Why Forbidden? |
| :--- | :--- | :--- | :--- |
| **Brackets / Braces** | `[`, `]`, `{`, `}` | `[1 + 2] * 3` | Only round parentheses `( )` are permitted for grouping. |
| **Bitwise / Logical** | `&`, `\|`, `~`, `<<`, `>>` | `5 & 3`, `1 \|\| 2` | Bitwise and Boolean logic operators are not implemented. |
| **Comparison** | `<`, `>`, `<=`, `>=`, `==`, `!=` | `5 > 3`, `x == 10` | The calculator evaluates arithmetic, not relational predicates. |
| **Special Symbols** | `@`, `$`, `#`, `\`, `;`, `:`, `'`, `"` | `2 @ 3`, `$100 + 5` | Non-mathematical characters. |
| **Thousand Comma** | `,` within numbers | `1,000,000` | Comma is strictly a function argument separator, not a number delimiter. |

---

## 4. Number Formatting Rules

### 4.1 Integers (`BigInteger`)
* **Precision:** Unbounded arbitrary precision (limited solely by heap memory).
* **Base Packaging:** Grouped in base-$10^9$ limbs (9 decimal digits per 32-bit integer).
* **Leading Zeroes:** Allowed and normalized automatically (`007` $\longrightarrow 7$, `000` $\longrightarrow 0$).
* **Sign:** Canonical zero invariant enforced (`sign = 0, size = 1, digits[0] = 0`).

### 4.2 Decimals (`BigDecimal`)
* **Format:** Requires digits on at least one side of the decimal point (`.`):
  * **Valid:** `0.5`, `.5`, `123.456`, `0.00000000000000000001`
  * **Invalid:** `5.` (trailing dot with no succeeding digit is rejected), `.` (lone dot is rejected).
* **Multiple Dots:** `1.2.3` is rejected (parsed as `1.2` followed by `.3` with no intervening operator).

### 4.3 Scientific Notation
* **Format:** `<significand>[eE][+-]?<integer_exponent>`
  * **Valid:** `1e6`, `2.5e-3`, `1E+10`, `.5e4`
  * **Invalid:** `1e` (missing exponent), `1e+` (missing exponent digits), `1e2.5` (fractional exponents in scientific notation are invalid).

### 4.4 Summary Table: Number Formatting

| Number Input | Status | Result / Error | Reason |
| :--- | :---: | :--- | :--- |
| `12345678901234567890` | **VALID** | Exact integer | Unbounded integer digits supported |
| `007` | **VALID** | `= 7` | Leading zeroes stripped by normalizer |
| `0.5` | **VALID** | `= 0.5` | Standard decimal notation |
| `.5` | **VALID** | `= 0.5` | Leading zero before dot is optional |
| `5.` | **INVALID** | `Error: Incorrect expression` | Trailing dot without fractional digits is illegal |
| `.` | **INVALID** | `Error: Incorrect expression` | Lone dot is not a number |
| `1.2.3` | **INVALID** | `Error: Incorrect expression` | Multiple decimal points |
| `1,000` | **INVALID** | `Error: Incorrect expression` | Comma cannot be used as thousand separator |
| `1e5` | **VALID** | `= 100000` | Scientific integer notation |
| `1.5e-3` | **VALID** | `= 0.0015` | Scientific decimal notation |
| `1e` | **INVALID** | `Error: Incorrect expression` | Missing exponent digits |
| `1e+` | **INVALID** | `Error: Incorrect expression` | Missing exponent digits after sign |

---

## 5. Variables, Identifiers & Built-in Constants

### 5.1 Naming Rules
* Variable names must start with an ASCII letter (`a-z`, `A-Z`) or an underscore (`_`).
* Subsequent characters can be letters, digits (`0-9`), or underscores (`_`).
* **Regex:** `^[a-zA-Z_][a-zA-Z0-9_]*$`

### 5.2 Preloaded Mathematical Constants

| Constant | Data Type | Default Value / Formula | Notes |
| :--- | :--- | :--- | :--- |
| `pi` | `BigDecimal` | $3.14159265358979323846\dots$ | Computed dynamically via Ramanujan/Chudnovsky-class series to current precision + 8 guard digits. |
| `e` | `BigDecimal` | $2.71828182845904523536\dots$ | Computed via Taylor series expansion of $\exp(1)$. |
| `i` | `Complex` | $0 + 1i$ | Fundamental imaginary unit ($i^2 = -1$). |
| `ans` | Dynamic | Starts at `0` | Automatically updated after every successful calculation with the last computed value. |

### 5.3 Assignment Syntax (REPL Only)
* Format: `<variable_name> = <expression>`
* **Valid:**
  * `x = 10`
  * `y = x * 2 + 5`
  * `circle_area = pi * 5^2`
  * `ans_backup = ans`
* **Invalid:**
  * `x = y = 2` $\longrightarrow$ Chained assignment is rejected (`=` is illegal in expressions).
  * `2 = x` $\longrightarrow$ Target must be a valid identifier.
  * `x + 1 = 5` $\longrightarrow$ Target must be a single variable, not an expression.
  * `x =` $\longrightarrow$ Right-hand side expression cannot be empty.

### 5.4 Undefined Variable Reference
* Referencing a variable that has not been assigned triggers an immediate evaluation error:
  * Input: `foo + 1`
  * Output: `Error: Undefined variable 'foo'`

---

## 6. Operator Grammar, Precedence & Associativity

The recursive-descent parser evaluates expressions strictly according to the following 6-level structural hierarchy:

```
┌────────────────────────────────────────────────────────────────────────┐
│                       Operator Precedence Ladder                       │
├───────┬──────────────────────┬───────────────┬─────────┬───────────────┤
│ Level │ Operator             │ Associativity │ Arity   │ Classification│
├───────┼──────────────────────┼───────────────┼─────────┼───────────────┤
│   6   │ ( ), fn(...)         │ None          │ Primary │ Atomic / Call │
│   5   │ ! (Factorial)        │ Left (Postfix)│ Unary   │ Postfix       │
│   4   │ +, - (Unary Sign)    │ Right (Prefix)│ Unary   │ Prefix Sign   │
│   3   │ ^ (Exponentiation)   │ Right-to-Left │ Binary  │ Power         │
│   2   │ *, /, %              │ Left-to-Right │ Binary  │ Multiplicative│
│   1   │ +, - (Binary Add/Sub)│ Left-to-Right │ Binary  │ Additive      │
└───────┴──────────────────────┴───────────────┴─────────┴───────────────┘
```

### 6.1 Critical Precedence & Associativity Rules

1. **Right-Associativity of Exponentiation (`^`):**
   * Exponentiation groups right-to-left:
     $$2 \wedge 3 \wedge 2 \equiv 2 \wedge (3 \wedge 2) = 2^9 = 512$$
   * Evaluated as $512$, **not** $(2^3)^2 = 64$.
2. **Left-Associativity of Additive and Multiplicative Operators:**
   * $10 - 4 - 2 \equiv (10 - 4) - 2 = 4$ (not $10 - (4 - 2) = 8$).
   * $24 / 4 / 2 \equiv (24 / 4) / 2 = 3$ (not $24 / (4 / 2) = 12$).
3. **Prefix Unary Sign vs. Exponentiation:**
   * In this grammar, the unary level is called directly by the power parser. Therefore:
     $$-3 \wedge 2 \equiv (-3)^2 = 9$$
   * If you intend $-(3^2) = -9$, you must use explicit grouping parentheses: `-(3^2)`.
4. **Postfix Factorial vs. Unary Negation:**
   * Postfix `!` operates at Level 5, binding tighter than prefix negation at Level 4:
     $$-3! \equiv -(3!) = -(6) = -6$$
   * To evaluate the factorial of a negative integer (which is mathematically undefined), `(-3)!` is required, which correctly triggers:
     `Error: Factorial input negative or too large`.

---

## 7. Consecutive Operators & Disambiguation Rules

The lexer and parser disambiguate unary signs (`+`, `-`) from binary operators (`+`, `-`) based strictly on grammar position without backtracking.

### 7.1 Valid Consecutive Operator Combinations

| Combination | Input Example | Parsed Representation | Computed Result | Explanation |
| :--- | :--- | :--- | :---: | :--- |
| **Binary `+` and Unary `-`** | `2 + - 3` | `2 + (-3)` | `-1` | Second operator is treated as unary prefix sign |
| **Binary `+` and Unary `+`** | `2 + + 3` | `2 + (+3)` | `5` | Second operator is treated as unary positive sign |
| **Binary `-` and Unary `-`** | `2 - - 3` | `2 - (-3)` | `5` | Subtraction of a negative number |
| **Binary `*` and Unary `-`** | `4 * - 2` | `4 * (-2)` | `-8` | Multiplication by a negative number |
| **Binary `/` and Unary `-`** | `10 / - 2` | `10 / (-2)` | `-5` | Division by a negative number |
| **Binary `^` and Unary `-`** | `2 ^ - 3` | `2 ^ (-3)` | `0.125` | Negative power auto-promotes to decimal reciprocal |
| **Chained Unary Signs** | `- - - 5` | `-(-(-5))` | `-5` | Odd count of negative signs evaluates to negative |
| **Chained Mixed Signs** | `+ - + - 5` | `+(-(+(-5)))`| `5` | Two negative signs cancel to positive |
| **Chained Postfix Factorials** | `3!!` | `(3!)!` | `720` | $(3!)! = 6! = 720$ |

### 7.2 Invalid Consecutive Operator Combinations

| Combination | Input Example | Error Produced | Reason for Rejection |
| :--- | :--- | :--- | :--- |
| **Double Multiply** | `2 * * 3` | `Error: Incorrect expression` | `*` cannot act as a unary prefix operator. (Use `^` for power). |
| **Double Divide** | `10 / / 2` | `Error: Incorrect expression` | `/` cannot act as a unary prefix operator. |
| **Double Modulo** | `10 % % 3` | `Error: Incorrect expression` | `%` cannot act as a unary prefix operator. |
| **Double Exponent** | `2 ^ ^ 3` | `Error: Incorrect expression` | `^` cannot act as a unary prefix operator. |
| **Prefix Factorial** | `!5` | `Error: Incorrect expression` | `!` is strictly a postfix operator; it requires a preceding primary operand. |
| **Dangling Operator** | `5 +` | `Error: Incorrect expression` | Binary operator missing right-hand operand. |
| **Leading Binary Op** | `* 5` or `/ 5` | `Error: Incorrect expression` | Binary operator missing left-hand operand. |

---

## 8. Parentheses & Nesting Rules

### 8.1 Structural Invariants
1. **Strict Balance:** Every opening parenthesis `(` must have a matching closing parenthesis `)`.
   * Unclosed `(`: `2 * (3 + 4` $\longrightarrow$ `Error: Incorrect expression`
   * Stray `)`: `2 + 3)` $\longrightarrow$ `Error: Incorrect expression`
2. **Non-Empty Content:** Parentheses must wrap an expression.
   * `()` $\longrightarrow$ `Error: Incorrect expression`
   * `5 + ()` $\longrightarrow$ `Error: Incorrect expression`
3. **Explicit Subtree Scoping:** Parentheses override all default precedence rules:
   * `(2 + 3) * 4` evaluates as $5 \times 4 = 20$.
   * `2 ^ (3 ^ 2)` evaluates as $2^9 = 512$.
   * `(2 ^ 3) ^ 2` evaluates as $8^2 = 64$.

### 8.2 Maximum Recursion Depth Limit
To prevent OS stack overflow from adversarial or deeply nested inputs (e.g., $1+(1+(1+\dots))$), the AST tree walker enforces:
$$\text{MAX\_RECURSION\_DEPTH} = 128$$
* **Valid:** Up to 128 nested parenthetical layers.
* **Invalid:** $> 128$ nested parentheses triggers:
  `Error: Recursion depth limit exceeded`

---

## 9. Strict Prohibition of Implicit Multiplication

> [!IMPORTANT]
> **The calculator strictly requires explicit multiplication (`*`).**  
> Implicit or juxtaposed multiplication is NOT supported in any form and will produce a syntax error.

### 9.1 Invalid vs. Valid Syntax Comparison

| Intended Calculation | INVALID Syntax (Rejected) | VALID Syntax (Required) | Output |
| :--- | :--- | :--- | :--- |
| **Number and Parenthesis** | `2(3)` | `2 * (3)` | `= 6` |
| **Adjacent Parentheses** | `(2)(3)` | `(2) * (3)` | `= 6` |
| **Number and Variable** | `2x` | `2 * x` | Depends on `x` |
| **Number and Constant** | `2pi` | `2 * pi` | `= 6.28318530717958647692` |
| **Number and Imaginary Unit** | `3i` | `3 * i` | `= 3i` |
| **Variable and Parenthesis** | `x(y + 1)` | `x * (y + 1)` | Parsed as function call if `*` omitted! |
| **Parenthesis and Function** | `(2)sin(pi)` | `(2) * sin(pi)` | `= 0.0` |
| **Coefficient and Function** | `5 sin(x)` | `5 * sin(x)` | Depends on `x` |
| **Parenthesized Expressions** | `(a + b)(c + d)` | `(a + b) * (c + d)` | Exact product |

---

## 10. Built-in Mathematical Functions Specification

The engine includes 10 registered functions with strict arity (argument count) and domain requirements:

| Function | Arity | Argument Type | Supported Domain | Output Type | Description / Mathematical Identity |
| :--- | :---: | :--- | :--- | :--- | :--- |
| **`sin(x)`** | 1 | Integer, Decimal | $(-\infty, +\infty)$ | `BigDecimal` | Sine. Reduced modulo $2\pi$ via Maclaurin series. Respects active `rad` / `deg` mode. |
| **`cos(x)`** | 1 | Integer, Decimal | $(-\infty, +\infty)$ | `BigDecimal` | Cosine. Reduced modulo $2\pi$ via Maclaurin series. Respects active `rad` / `deg` mode. |
| **`exp(x)`** | 1 | Integer, Decimal | $(-\infty, +\infty)$ | `BigDecimal` | Exponential $e^x$. Integer/fraction split Taylor series. |
| **`ln(x)`** | 1 | Integer, Decimal | $x > 0$ | `BigDecimal` | Natural logarithm. Power-of-10 reduction + series. ($x \le 0$ is rejected). |
| **`sqrt(x)`**| 1 | Integer, Decimal | $x \ge 0$ | `BigDecimal` | Square root. High-precision Newton-Raphson iteration. ($x < 0$ is rejected). |
| **`abs(x)`** | 1 | Int, Dec, Complex | All numbers | Same / Dec | Absolute value: $\lvert -5\rvert = 5$, $\lvert -3.14\rvert = 3.14$, $\lvert 3+4i\rvert = \sqrt{a^2+b^2} = 5.0$. |
| **`gcd(a, b)`**| 2 | Integer only | $\mathbb{Z} \times \mathbb{Z}$ | `BigInteger` | Greatest Common Divisor via Euclidean algorithm. Supports negative integers. |
| **`pow(a, b)`**| 2 | Any real pair | Same as `a ^ b` | Int / Dec | Direct functional alias for binary exponentiation `a ^ b`. |
| **`mod(a, b)`**| 2 | Integer only | $b \ne 0$ | `BigInteger` | Direct functional alias for binary modulo `a % b`. |
| **`fact(n)`** | 1 | Integer only | $0 \le n \le 10000$| `BigInteger` | Direct functional alias for postfix factorial `n!`. |

### 10.1 Function Syntax Validation Rules

1. **Parentheses are Mandatory:**
   * `sin(5)` $\longrightarrow$ **VALID**
   * `sin 5` $\longrightarrow$ **INVALID** (`Error: Incorrect expression`)
2. **Arity Must Match Exactly:**
   * `sin()` $\longrightarrow$ **INVALID** (Expected 1 argument, got 0)
   * `sin(1, 2)` $\longrightarrow$ **INVALID** (Expected 1 argument, got 2)
   * `gcd(48)` $\longrightarrow$ **INVALID** (Expected 2 arguments, got 1)
   * `gcd(48, 18, 6)` $\longrightarrow$ **INVALID** (Expected 2 arguments, got 3)
3. **Unregistered Functions are Rejected:**
   * `tan(x)`, `atan(x)`, `asin(x)`, `acos(x)`, `log10(x)`, `ceil(x)`, `floor(x)` are **not** present in the function catalog. Calling them produces:
     `Error: Incorrect expression` (Unknown function name at parse time).

---

## 11. Type System & Automatic Promotion Hierarchy

The runtime evaluator categorizes values into a tagged union hierarchy with three numerical types:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Dynamic Type Promotion                          │
│                                                                        │
│       VAL_INT                VAL_DEC               VAL_COMPLEX         │
│     BigInteger     ───►    BigDecimal     ───►       Complex           │
│   (Exact Limbs)         (Significand+Scale)         (Real + Imag)      │
└────────────────────────────────────────────────────────────────────────┘
```

### 11.1 Promotion Rules

1. **Mixed Operand Widening:**
   When binary operators encounter operands of different types, the narrower type is automatically widened to the wider type before evaluation:
   $$\text{VAL\_INT} + \text{VAL\_DEC} \Longrightarrow \text{VAL\_DEC} + \text{VAL\_DEC}$$
   $$\text{VAL\_DEC} + \text{VAL\_COMPLEX} \Longrightarrow \text{VAL\_COMPLEX} + \text{VAL\_COMPLEX}$$
   $$\text{VAL\_INT} \times \text{VAL\_COMPLEX} \Longrightarrow \text{VAL\_COMPLEX} \times \text{VAL\_COMPLEX}$$
2. **Automatic Division Promotion:**
   * If integer division $a / b$ has a non-zero remainder ($a \pmod b \ne 0$), the result is **automatically promoted to `BigDecimal`** and evaluated to the active decimal precision:
     $$10 / 4 \Longrightarrow 2.50000000000000000000$$
   * If division is exact, it remains a `BigInteger`:
     $$10 / 2 \Longrightarrow 5$$
3. **Automatic Negative Exponent Promotion:**
   * An integer raised to a negative integer exponent ($b^{-e}$) automatically promotes to `BigDecimal`:
     $$2^{-3} \Longrightarrow \frac{1}{2^3} = 0.12500000000000000000$$
4. **Imaginary Arithmetic Promotion:**
   * Combining any real value with `i` elevates the expression to `Complex`:
     $$2 + 3 * i \Longrightarrow 2 + 3i$$

---

## 12. Type-by-Type Operational Support Matrix

Not all operators and functions are defined for every numerical type. The table below details what operations are valid across types:

| Operator / Function | Integer (`VAL_INT`) | Decimal (`VAL_DEC`) | Complex (`VAL_COMPLEX`) | Failure Message if Unsupported |
| :--- | :---: | :---: | :---: | :--- |
| **`+` (Addition)** | Supported | Supported | Supported | — |
| **`-` (Subtraction)** | Supported | Supported | Supported | — |
| **`*` (Multiplication)** | Supported | Supported | Supported | — |
| **`/` (Division)** | Supported | Supported | Supported (Conjugate) | — |
| **`%` or `mod` (Modulo)** | Supported | **UNSUPPORTED** | **UNSUPPORTED** | `Error: Unsupported operator on decimal` / `on complex` |
| **`^` or `pow` (Power)** | Supported | Supported ($base > 0$)| **UNSUPPORTED** | `Error: Unsupported operator on complex` |
| **`!` or `fact` (Factorial)**| Supported ($0 \le n \le 10000$)| **UNSUPPORTED** | **UNSUPPORTED** | `Error: Factorial requires non-negative integer` |
| **`abs` (Absolute Value)** | Supported | Supported | Supported ($|z| = \sqrt{a^2+b^2}$)| — |
| **`gcd` (Greatest Common Div)**| Supported | **UNSUPPORTED** | **UNSUPPORTED** | `Error: gcd requires integer arguments` |
| **`sqrt` (Square Root)** | Promotes to Dec | Supported ($x \ge 0$) | **UNSUPPORTED** | `Error: sqrt requires numeric argument` |
| **`ln` (Natural Log)** | Promotes to Dec | Supported ($x > 0$) | **UNSUPPORTED** | `Error: ln requires numeric argument` |
| **`exp` (Exponential)** | Promotes to Dec | Supported | **UNSUPPORTED** | `Error: exp requires numeric argument` |
| **`sin` / `cos` (Trig)** | Promotes to Dec | Supported | **UNSUPPORTED** | `Error: sin/cos requires numeric argument` |

---

## 13. Mathematical Domain Limits & Guard Boundaries

| Operation / Feature | Boundary Condition | Result if Boundary Exceeded | Diagnostic Error Emitted |
| :--- | :--- | :--- | :--- |
| **Division by Zero** | Denominator is zero ($x / 0$) | Aborted cleanly | `Error: Division by zero` |
| **Modulo by Zero** | Modulus is zero ($x \% 0$) | Aborted cleanly | `Error: Modulo by zero` |
| **Complex Div by Zero** | Divisor $c + di$ has $c^2 + d^2 = 0$ | Aborted cleanly | `Error: Division by complex zero` |
| **Negative Square Root** | $x < 0$ in $\text{sqrt}(x)$ | Aborted cleanly | `Error: Domain error: negative sqrt argument` |
| **Non-positive Logarithm**| $x \le 0$ in $\ln(x)$ | Aborted cleanly | `Error: Domain error: ln argument <= 0` |
| **Factorial Sign** | $n < 0$ in $n!$ or $\text{fact}(n)$ | Aborted cleanly | `Error: Factorial input negative or too large` |
| **Factorial Upper Limit** | $n > 10,000$ in $n!$ | Aborted cleanly (time guard) | `Error: Factorial input negative or too large` |
| **Decimal Power Base** | Base $\le 0$ with decimal exponent | Aborted cleanly | `Error: Negative base for decimal power` |
| **Zero to Negative Power**| $0^{-k}$ where $k > 0$ | Evaluates $1 / 0$ | Rejects with division by zero |
| **Zero to Zero Power** | $0^0$ | Exact mathematical convention | `= 1` |
| **Decimal Precision Range**| REPL `precision <n>` | Allowed: $0 \le n \le 2000$ | `[Error: precision must be between 0 and 2000]` |
| **Max Recursion Depth** | Expression nesting depth $> 128$ | Aborts to prevent stack overflow| `Error: Recursion depth limit exceeded` |

---

## 14. REPL Interactive Commands

When running in interactive REPL mode (`./calculator`), lines that match internal control commands are executed without passing through the mathematical parser:

| Command | Arguments / Options | Description | Example Usage |
| :--- | :--- | :--- | :--- |
| **`exit`, `quit`, `q`** | None | Terminates the calculator REPL session cleanly. | `exit` |
| **`help`, `?`** | None | Displays help banner and operator syntax summary. | `help` |
| **`precision`** | None | Prints the current decimal precision setting. | `precision` $\longrightarrow$ `[Current precision: 20]` |
| **`precision <n>`** | Integer $n \in [0, 2000]$ | Sets decimal precision for divisions, square roots, and series. Recomputes constants (`pi`, `e`). | `precision 50` $\longrightarrow$ `[Precision set to 50 decimal places]` |
| **`mode`** | None | Toggles angular mode between `RADIAN` and `DEGREE`. | `mode` $\longrightarrow$ `[Angle mode is now DEGREE]` |
| **`mode rad`** | `rad` or `radian` | Explicitly sets angular mode to radians. | `mode rad` |
| **`mode deg`** | `deg` or `degree` | Explicitly sets angular mode to degrees. | `mode deg` |
| **`vars`** | None | Lists all currently defined symbols and constants (`pi`, `e`, `i`, `ans`, user variables). | `vars` |
| *(Empty line)* | None | Ignored silently; prompts user for next line. | *(Press Enter)* |

---

## 15. Master Valid vs. Invalid Combinations Matrix

The following comprehensive table classifies all combinations of inputs, syntax patterns, and edge cases:

| ID | Input Expression | Status | Evaluated Result or Diagnostic Output | Underlying Rule / Reason |
| :---: | :--- | :---: | :--- | :--- |
| **A01** | `2 + 3 * 4` | **VALID** | `= 14` | Multiplication precedes addition. |
| **A02** | `(2 + 3) * 4` | **VALID** | `= 20` | Parentheses override precedence. |
| **A03** | `2 ^ 3 ^ 2` | **VALID** | `= 512` | Right-associative: $2^{(3^2)} = 2^9 = 512$. |
| **A04** | `(2 ^ 3) ^ 2` | **VALID** | `= 64` | Explicit parentheses force left association: $8^2 = 64$. |
| **A05** | `10 - 4 - 2` | **VALID** | `= 4` | Left-associative: $(10 - 4) - 2 = 4$. |
| **A06** | `10 / 4` | **VALID** | `= 2.50000000000000000000` | Integer division auto-promotes to decimal on non-zero remainder. |
| **A07** | `10 / 2` | **VALID** | `= 5` | Exact division retains `BigInteger` type. |
| **A08** | `10 % 3` | **VALID** | `= 1` | Exact integer modulo. |
| **A09** | `10 % -3` | **VALID** | `= 1` | Modulo with negative divisor. |
| **A10** | `-10 % 3` | **VALID** | `= -1` | Modulo with negative dividend preserves dividend sign. |
| **A11** | `0 / 5` | **VALID** | `= 0` | Zero divided by positive integer. |
| **A12** | `0 % 5` | **VALID** | `= 0` | Zero modulo positive integer. |
| **A13** | `10 / 0` | **INVALID**| `Error: Division by zero` | Division by zero is mathematically undefined. |
| **A14** | `10 % 0` | **INVALID**| `Error: Modulo by zero` | Modulo by zero is mathematically undefined. |
| **A15** | `0 / 0` | **INVALID**| `Error: Division by zero` | Indeterminate form rejected. |
| **B01** | `+5` | **VALID** | `= 5` | Unary positive prefix. |
| **B02** | `-5` | **VALID** | `= -5` | Unary negative prefix. |
| **B03** | `- - 5` | **VALID** | `= 5` | Consecutive unary minus signs cancel. |
| **B04** | `- - - 5` | **VALID** | `= -5` | Chained unary minus signs. |
| **B05** | `2 + - 3` | **VALID** | `= -1` | Positional unary minus after binary addition. |
| **B06** | `2 * - 3` | **VALID** | `= -6` | Positional unary minus after binary multiplication. |
| **B07** | `2 / - 4` | **VALID** | `= -0.50000000000000000000` | Unary minus after division with auto-promotion. |
| **B08** | `2 ^ - 3` | **VALID** | `= 0.12500000000000000000` | Negative exponent promotes to decimal reciprocal. |
| **B09** | `-3 ^ 2` | **VALID** | `= 9` | Unary sign binds to base before `^` in this grammar. |
| **B10** | `-(3 ^ 2)` | **VALID** | `= -9` | Parentheses isolate negation outside power. |
| **B11** | `(-3) ^ 3` | **VALID** | `= -27` | Odd power of negative integer. |
| **B12** | `(-3) ^ 2` | **VALID** | `= 9` | Even power of negative integer. |
| **B13** | `2 * * 3` | **INVALID**| `Error: Incorrect expression` | Double multiplication operator is invalid syntax. |
| **B14** | `2 / / 3` | **INVALID**| `Error: Incorrect expression` | Double division operator is invalid syntax. |
| **B15** | `2 ^ ^ 3` | **INVALID**| `Error: Incorrect expression` | Double exponentiation operator is invalid syntax. |
| **B16** | `* 5` | **INVALID**| `Error: Incorrect expression` | Binary operator missing left operand. |
| **B17** | `5 *` | **INVALID**| `Error: Incorrect expression` | Binary operator missing right operand. |
| **C01** | `0!` | **VALID** | `= 1` | Mathematical definition: $0! = 1$. |
| **C02** | `5!` | **VALID** | `= 120` | Postfix factorial. |
| **C03** | `3!!` | **VALID** | `= 720` | Chained factorial: $(3!)! = 6! = 720$. |
| **C04** | `-3!` | **VALID** | `= -6` | Postfix `!` binds tighter than prefix `-`: $-(3!) = -6$. |
| **C05** | `(-3)!` | **INVALID**| `Error: Factorial input negative or too large`| Factorial of negative integer is undefined. |
| **C06** | `5.5!` | **INVALID**| `Error: Factorial requires non-negative integer`| Factorial requires integer operand. |
| **C07** | `!5` | **INVALID**| `Error: Incorrect expression` | Factorial is strictly postfix; cannot precede number. |
| **C08** | `10000!` | **VALID** | Exact ($35,660$ digits) | Upper bound for factorial evaluation. |
| **C09** | `10001!` | **INVALID**| `Error: Factorial input negative or too large`| Exceeds factorial safety guard limit ($N \le 10000$). |
| **C10** | `fact(5)` | **VALID** | `= 120` | Functional alias for factorial. |
| **D01** | `0 ^ 0` | **VALID** | `= 1` | Standard computing convention $0^0 = 1$. |
| **D02** | `0 ^ 5` | **VALID** | `= 0` | Zero raised to positive integer power. |
| **D03** | `5 ^ 0` | **VALID** | `= 1` | Non-zero number raised to zero power. |
| **D04** | `0 ^ -2` | **INVALID**| `Error: Division by zero` | Equivalent to $1 / 0^2 = 1 / 0$. |
| **D05** | `2 ^ -3.5` | **VALID** | `= 0.08838834764831844055` | Decimal power computed via $\exp(-3.5 \times \ln(2))$. |
| **D06** | `(-2) ^ 0.5` | **INVALID**| `Error: Negative base for decimal power`| Fractional power of negative base requires complex power engine. |
| **E01** | `2(3)` | **INVALID**| `Error: Incorrect expression` | Implicit multiplication forbidden (must be `2 * (3)`). |
| **E02** | `(2)(3)` | **INVALID**| `Error: Incorrect expression` | Implicit multiplication forbidden (must be `(2) * (3)`). |
| **E03** | `2x` | **INVALID**| `Error: Incorrect expression` | Implicit multiplication forbidden (must be `2 * x`). |
| **E04** | `2pi` | **INVALID**| `Error: Incorrect expression` | Implicit multiplication forbidden (must be `2 * pi`). |
| **E05** | `3i` | **INVALID**| `Error: Incorrect expression` | Implicit multiplication forbidden (must be `3 * i`). |
| **E06** | `sin 5` | **INVALID**| `Error: Incorrect expression` | Function calls require parentheses `sin(5)`. |
| **E07** | `5 sin(x)` | **INVALID**| `Error: Incorrect expression` | Must write `5 * sin(x)`. |
| **F01** | `(1 + 2` | **INVALID**| `Error: Incorrect expression` | Unclosed opening parenthesis. |
| **F02** | `1 + 2)` | **INVALID**| `Error: Incorrect expression` | Unmatched closing parenthesis. |
| **F03** | `()` | **INVALID**| `Error: Incorrect expression` | Empty parentheses are invalid. |
| **F04** | `1 + (2 * )` | **INVALID**| `Error: Incorrect expression` | Missing operand inside parenthetical sub-expression. |
| **F05** | Nested 129 `(` | **INVALID**| `Error: Recursion depth limit exceeded` | Exceeds `MAX_RECURSION_DEPTH = 128`. |
| **G01** | `sin(0)` | **VALID** | `= 0` | Sine of 0. |
| **G02** | `cos(0)` | **VALID** | `= 1` | Cosine of 0. |
| **G03** | `sin(pi / 2)` | **VALID** | `= 1.00000000000000000000` | Radian sine of $\pi/2$. |
| **G04** | `exp(0)` | **VALID** | `= 1` | Exponential $e^0 = 1$. |
| **G05** | `exp(1)` | **VALID** | `= 2.71828182845904523536` | Euler's number $e^1$. |
| **G06** | `ln(1)` | **VALID** | `= 0` | Natural logarithm $\ln(1) = 0$. |
| **G07** | `ln(0)` | **INVALID**| `Error: Domain error: ln argument <= 0`| Logarithm of zero is undefined ($-\infty$). |
| **G08** | `ln(-5)` | **INVALID**| `Error: Domain error: ln argument <= 0`| Logarithm of negative number is undefined in real domain. |
| **G09** | `sqrt(0)` | **VALID** | `= 0` | Square root of zero. |
| **G10** | `sqrt(144)` | **VALID** | `= 12.00000000000000000000` | Exact integer square root. |
| **G11** | `sqrt(-4)` | **INVALID**| `Error: Domain error: negative sqrt argument`| Real square root of negative number is undefined. |
| **G12** | `gcd(48, 18)` | **VALID** | `= 6` | Euclidean greatest common divisor. |
| **G13** | `gcd(-48, 18)` | **VALID** | `= 6` | GCD on negative integers. |
| **G14** | `gcd(48.5, 18)`| **INVALID**| `Error: gcd requires integer arguments`| GCD requires strictly integer arguments. |
| **G15** | `sin()` | **INVALID**| `Error: Incorrect expression` | Function arity mismatch (expected 1, got 0). |
| **G16** | `sin(1, 2)` | **INVALID**| `Error: Incorrect expression` | Function arity mismatch (expected 1, got 2). |
| **G17** | `gcd(10)` | **INVALID**| `Error: Incorrect expression` | Function arity mismatch (expected 2, got 1). |
| **G18** | `tan(0)` | **INVALID**| `Error: Incorrect expression` | Unknown function name. |
| **H01** | `i * i` | **VALID** | `= -1` | Imaginary identity $i^2 = -1$. |
| **H02** | `2 + 3 * i` | **VALID** | `= 2 + 3i` | Complex number construction. |
| **H03** | `(2 + 3*i) + (4 - 5*i)`| **VALID** | `= 6 - 2i` | Complex addition. |
| **H04** | `(2 + 3*i) * (2 - 3*i)`| **VALID** | `= 13` | Conjugate product $(a^2 + b^2)$. |
| **H05** | `(1 + i) / (1 - i)` | **VALID** | `= 1.00000000000000000000i` | Complex conjugate division. |
| **H06** | `abs(3 + 4*i)` | **VALID** | `= 5.00000000000000000000` | Complex modulus $\sqrt{3^2 + 4^2} = 5$. |
| **H07** | `(2 + 3*i) % 2`| **INVALID**| `Error: Unsupported operator on complex`| Modulo is not defined on complex numbers. |
| **H08** | `i ^ 2` | **INVALID**| `Error: Unsupported operator on complex`| Complex power is not implemented (use `i * i`). |
| **H09** | `sin(i)` | **INVALID**| `Error: sin requires numeric argument` | Trigonometric functions require real numeric arguments. |
| **H10** | `sqrt(i)` | **INVALID**| `Error: sqrt requires numeric argument`| Square root requires real numeric argument. |
| **I01** | `0.1 + 0.2` | **VALID** | `= 0.3` | Exact arbitrary-precision decimal addition (no IEEE 754 float drift). |
| **I02** | `5.5 % 2` | **INVALID**| `Error: Unsupported operator on decimal`| Modulo requires integer operands. |
| **I03** | `mod(5.5, 2)` | **INVALID**| `Error: Unsupported operator on decimal`| Modulo requires integer operands. |
| **J01** | `1,234` | **INVALID**| `Error: Incorrect expression` | Comma cannot be used as a thousands separator. |
| **J02** | `1; 2` | **INVALID**| `Error: Incorrect expression` | Semicolon is illegal inside an expression. |
| **J03** | `2 @ 3` | **INVALID**| `Error: Incorrect expression` | Disallowed character `@`. |
| **J04** | `""` (Empty) | **INVALID**| `Error: Incorrect expression` | Empty expression is rejected. |
| **J05** | `"   "` | **INVALID**| `Error: Incorrect expression` | Whitespace-only expression is rejected. |

---

## 16. Diagnostic Error Code Reference

When an expression is rejected, the calculator outputs an informative diagnostic message. The table below maps each error message to its root cause and the required remedy:

| Displayed Error Message | Diagnostic Category | Typical Root Cause | How to Fix |
| :--- | :--- | :--- | :--- |
| `Error: Incorrect expression` | Syntax / Lexer / Parser | Disallowed character, unclosed parenthesis, missing operator, missing operand, implicit multiplication, or unknown function name. | Ensure all parentheses balance, use `*` explicitly (e.g. `2 * (3)`), and check that all function names and characters are valid. |
| `Error: Division by zero` | Evaluation Runtime | Denominator evaluated to zero (`/ 0`). | Verify denominator expression cannot equal zero. |
| `Error: Modulo by zero` | Evaluation Runtime | Modulus evaluated to zero (`% 0`). | Ensure divisor in `%` or `mod()` is non-zero. |
| `Error: Division by complex zero` | Evaluation Runtime | Complex denominator $c + di$ has $c = 0$ and $d = 0$. | Ensure complex denominator is non-zero. |
| `Error: Domain error: negative sqrt argument` | Mathematical Domain | Attempted `sqrt(x)` where $x < 0$. | Ensure argument to `sqrt()` is non-negative ($x \ge 0$). |
| `Error: Domain error: ln argument <= 0` | Mathematical Domain | Attempted `ln(x)` where $x \le 0$. | Ensure argument to `ln()` is strictly positive ($x > 0$). |
| `Error: Factorial requires non-negative integer` | Type Mismatch | Attempted `x!` or `fact(x)` where $x$ is decimal or complex. | Supply a non-negative integer operand. |
| `Error: Factorial input negative or too large` | Domain / Upper Bound | Attempted $n!$ with $n < 0$ or $n > 10,000$. | Restrict factorial operand to the range $0 \le n \le 10,000$. |
| `Error: Negative base for decimal power` | Mathematical Domain | Attempted $b^e$ with $b \le 0$ and decimal $e$. | Decimal exponentiation requires a strictly positive base ($b > 0$). |
| `Error: Undefined variable '<name>'` | Symbol Table | Variable `<name>` was referenced before being assigned. | Assign variable in REPL (`x = <expr>`) prior to referencing it. |
| `Error: gcd requires integer arguments` | Type Mismatch | Passed decimal or complex arguments to `gcd()`. | Ensure both arguments to `gcd(a, b)` are integers. |
| `Error: Unsupported operator on decimal` | Type Hierarchy | Attempted `%` or `mod()` on `BigDecimal` values. | Modulo is strictly defined on integers (`BigInteger`). |
| `Error: Unsupported operator on complex` | Type Hierarchy | Attempted `%` or `^` on `Complex` values. | Only `+`, `-`, `*`, `/`, and `abs()` are supported on complex numbers. |
| `Error: sin requires numeric argument` | Type Mismatch | Passed complex number to trigonometric or series function. | Functions `sin`, `cos`, `exp`, `ln`, `sqrt` require real numbers. |
| `Error: Recursion depth limit exceeded` | Resource Limit | Parenthetical nesting depth exceeded 128 layers. | Simplify expression or reduce nested parentheses below 128. |
| `[Error: precision must be between 0 and 2000]` | Configuration | Passed $n < 0$ or $n > 2000$ to REPL `precision <n>`. | Choose a precision value in the inclusive range $[0, 2000]$. |

---

## 17. Summary Checklist for Expression Writers

Before submitting an expression to the calculator, verify:

1. [ ] **Explicit Operators:** Are all multiplications written with `*` (no `2(3)`, `2pi`, or `3i`)?
2. [ ] **Balanced Parentheses:** Does every `(` have a matching `)`?
3. [ ] **Allowed Characters Only:** Are all characters standard alphanumeric, whitespace, or recognized math operators (`+ - * / % ^ !`)?
4. [ ] **Valid Number Formats:** Do all decimal numbers have digits adjacent to the dot (e.g. `0.5`, not `5.`)?
5. [ ] **No Thousand Separators:** Are large numbers written continuously without commas (e.g. `1000000`, not `1,000,000`)?
6. [ ] **Function Syntax:** Are function arguments wrapped in parentheses (`sin(x)`, not `sin x`), and is the argument count correct?
7. [ ] **Domain Guards:** Are arguments to `sqrt` non-negative, `ln` strictly positive, and denominators non-zero?
8. [ ] **Integer Restrictions:** Are modulo (`%`) and GCD (`gcd`) applied exclusively to integers?
9. [ ] **Factorial Limits:** Is factorial applied to integers satisfying $0 \le n \le 10,000$?
10. [ ] **Complex Number Restrictions:** Are operations on complex numbers limited to `+`, `-`, `*`, `/`, and `abs()`?
