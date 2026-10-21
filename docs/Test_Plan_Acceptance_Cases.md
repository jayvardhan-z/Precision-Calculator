# Test Plan & Acceptance Test Cases

**Project Name:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Project Identifier:** Precision-Calculator  
**Version:** 2.0.0  
**Document Classification:** Quality Assurance & Verification Plan  

---

## 1. Test Strategy & Scope

The testing strategy validates the correctness, numerical stability, performance bounds, and memory safety of the Precision-Calculator engine across three tiers:
1. **Unit Testing:** Direct functional testing of internal algorithms (Karatsuba, Knuth Algorithm D, Newton-Raphson, limb normalization).
2. **End-to-End Infix Evaluation:** Verifying expression tokenization, AST generation, and type promotion via the CLI and REPL.
3. **Differential Oracle Testing:** Comparing calculator outputs against independent reference implementations (Python `decimal` / GNU `bc`) across millions of pseudorandomly generated inputs.
4. **Memory & Static Analysis:** Continuous validation under AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), and Valgrind Memcheck.

---

## 2. Test Suites & Test Cases

### Suite 1: Unbounded BigInteger Arithmetic
Validates base-$10^9$ limb addition, subtraction with borrow propagation, column & Karatsuba multiplication, and Knuth Algorithm D multi-limb division.

| Case ID | Input Expression | Expected Output | Verification Target |
| :--- | :--- | :--- | :--- |
| **TC-INT-01** | `999999999 + 1` | `1000000000` | Single-limb carry overflow to next limb |
| **TC-INT-02** | `1000000000 - 1` | `999999999` | Multi-digit limb borrow propagation |
| **TC-INT-03** | `1000000000000000000 * 1000000000000000000` | `1000000000000000000000000000000000000` | Multi-limb product without 64-bit overflow |
| **TC-INT-04** | `2^64` | `18446744073709551616` | Exact binary repeated squaring exponentiation |
| **TC-INT-05** | `fact(12)` | `479001600` | Factorial calculation |
| **TC-INT-06** | `gcd(1071, 462)` | `21` | Euclidean greatest common divisor |
| **TC-INT-07** | `1000000000000000000 % 3` | `1` | Multi-limb modulo |
| **TC-INT-08** | `(10^50 + 7) * (10^50 - 7)` | `10^100 - 49` | Karatsuba crossover correctness ($> 32$ limbs) |

---

### Suite 2: Fixed-Point BigDecimal Arithmetic & Scale Alignment
Validates arbitrary-precision decimals, scale equalization, recurring divisions, and `ROUND_HALF_UP` tie-breaking.

| Case ID | Input Expression | Expected Output (Precision 20) | Verification Target |
| :--- | :--- | :--- | :--- |
| **TC-DEC-01** | `10 / 4` | `2.50000000000000000000` | Non-zero remainder integer auto-promotion |
| **TC-DEC-02** | `1 / 3` | `0.33333333333333333333` | Infinite recurring decimal truncation |
| **TC-DEC-03** | `1.2345 + 0.0000678` | `1.2345678` | Dynamic scale alignment ($\Delta s = 7 - 4 = 3$) |
| **TC-DEC-04** | `2.5 * 1.5` | `3.75` | Scale addition in multiplication ($s = 1 + 1 = 2$) |
| **TC-DEC-05** | `2 ^ (-3)` | `0.12500000000000000000` | Negative integer exponent promotion to decimal |

---

### Suite 3: Complex Number Arithmetic
Validates complex numbers ($z = a + bi$), conjugate division, and modulus operations.

| Case ID | Input Expression | Expected Output | Verification Target |
| :--- | :--- | :--- | :--- |
| **TC-CPX-01** | `i * i` | `-1` | Fundamental imaginary definition $i^2 = -1$ |
| **TC-CPX-02** | `(2 + 3*i) + (4 - 5*i)` | `6 - 2i` | Component-wise complex addition |
| **TC-CPX-03** | `(2 + 3*i) * (4 + 5*i)` | `-7 + 22i` | Complex multiplication: $(ac-bd) + (ad+bc)i$ |
| **TC-CPX-04** | `(1 + 2*i) / (1 - 2*i)` | `-0.60000000000000000000 + 0.80000000000000000000i` | Conjugate division scaled by $c^2 + d^2$ |
| **TC-CPX-05** | `abs(3 + 4*i)` | `5.00000000000000000000` | Complex modulus: $|z| = \sqrt{a^2 + b^2}$ |

---

### Suite 4: Transcendental & Scientific Functions
Validates Newton-Raphson square roots, range-reduced logarithms, exponentials, and trigonometric functions.

| Case ID | Input Expression | Expected Output (Precision 20) | Verification Target |
| :--- | :--- | :--- | :--- |
| **TC-FNC-01** | `sqrt(2)` | `1.41421356237309504880` | Newton-Raphson quadratic convergence |
| **TC-FNC-02** | `sqrt(144)` | `12.00000000000000000000` | Exact integer root convergence |
| **TC-FNC-03** | `exp(0)` | `1` | Exponential identity $e^0 = 1$ |
| **TC-FNC-04** | `exp(1)` | `2.71828182845904523536` | Euler's constant generation |
| **TC-FNC-05** | `ln(1)` | `0` | Logarithm identity $\ln(1) = 0$ |
| **TC-FNC-06** | `ln(100)` | `4.60517018598809136804` | Power-of-10 range reduction: $2 \times \ln(10)$ |
| **TC-FNC-07** | `sin(0)` | `0` | Trigonometric zero angle |
| **TC-FNC-08** | `cos(0)` | `1` | Cosine zero angle |
| **TC-FNC-09** | `sin(pi / 2)` | `1.00000000000000000000` | Right angle radian sine |
| **TC-FNC-10** | `cos(pi)` | `-1.00000000000000000000` | Inverted angle cosine |

---

### Suite 5: Lexer & Operator Precedence Grammar
Validates structural grammar precedence, associativity, and parentheses parsing.

| Case ID | Input Expression | Expected Output | Verification Target |
| :--- | :--- | :--- | :--- |
| **TC-GRM-01** | `2 + 3 * 4` | `14` | Multiplication precedes addition |
| **TC-GRM-02** | `(2 + 3) * 4` | `20` | Grouping parentheses override precedence |
| **TC-GRM-03** | `2 ^ 3 ^ 2` | `512` | Right-associativity of power: $2^{(3^2)}$ |
| **TC-GRM-04** | `-3^2` | `9` (or `-9` by grammar)| Positional unary sign binding |
| **TC-GRM-05** | `10 - 3 - 2` | `5` | Left-associativity of subtraction |

---

### Suite 6: Negative & Edge Case Diagnostics
Validates robust handling of pathological expressions, syntax errors, and domain errors without crashing.

| Case ID | Input Expression | Expected Diagnostic | Error Classification |
| :--- | :--- | :--- | :--- |
| **TC-ERR-01** | `10 / 0` | `Division by zero` | `ERR_DIV_BY_ZERO` |
| **TC-ERR-02** | `10 % 0` | `Modulo by zero` | `ERR_DIV_BY_ZERO` |
| **TC-ERR-03** | `sqrt(-4)` | `Domain error: negative sqrt argument` | `ERR_DOMAIN` |
| **TC-ERR-04** | `ln(-10)` | `Domain error: ln argument <= 0` | `ERR_DOMAIN` |
| **TC-ERR-05** | `2 * (3 + 4` | Caret under col 5: `unclosed '('` | Syntax error |
| **TC-ERR-06** | `unknown_var + 1` | `Undefined variable 'unknown_var'` | `ERR_UNDEFINED_VAR` |
| **TC-ERR-07** | `fact(-5)` | `Factorial requires non-negative integer` | `ERR_TYPE_MISMATCH` |
| **TC-ERR-08** | Nested 200 parentheses | `Recursion depth limit exceeded` | `ERR_RECURSION_LIMIT` |

---

## 3. Automated Test Execution Script

A bash test harness executes the verification suites in batch:

```bash
#!/usr/bin/env bash
# File: tests/run_acceptance_tests.sh
set -euo pipefail

CALC="./calculator"
FAILED=0

run_test() {
    local expr="$1"
    local expected="$2"
    local desc="$3"

    output=$($CALC "$expr" 2>&1 | grep '=' | awk '{$1=""; print $0}' | awk '{print $1}' || true)
    if [[ "$output" == *"$expected"* ]]; then
        echo -e "[\033[0;32mPASS\033[0m] $desc: '$expr' -> $output"
    else
        echo -e "[\033[0;31mFAIL\033[0m] $desc: '$expr' (Expected: '$expected', Got: '$output')"
        FAILED=$((FAILED + 1))
    fi
}

echo "=== Running Precision-Calculator Acceptance Suite ==="
run_test "2 + 3 * 4" "14" "Precedence Addition & Multiplication"
run_test "2 ^ 3 ^ 2" "512" "Right-associative Exponentiation"
run_test "10 / 4" "2.50000000000000000000" "Integer Division Auto-Promotion"
run_test "(2 + 3*i) * (4 + 5*i)" "-7" "Complex Number Real Part"
run_test "sqrt(144)" "12.00000000000000000000" "Newton-Raphson Sqrt"
run_test "sin(pi / 2)" "1.00000000000000000000" "Trigonometric Sine of Pi/2"
run_test "exp(1)" "2.71828182845904523536" "Euler's Constant Exp(1)"
run_test "gcd(48, 18)" "6" "Euclidean GCD"

if [ $FAILED -eq 0 ]; then
    echo -e "\n\033[0;32mALL ACCEPTANCE TESTS PASSED SUCCESSFULLY.\033[0m"
    exit 0
else
    echo -e "\n\033[0;31m$FAILED TEST(S) FAILED.\033[0m"
    exit 1
fi
```

---

## 4. QA Sign-Off Criteria Checklist

- [x] All 35+ test cases in Suites 1–6 pass with zero discrepancies.
- [x] Division of 100-digit by 200-digit number completes in $< 1.0\text{ s}$.
- [x] No memory leaks detected under `valgrind --leak-check=full`.
- [x] Warning-free compilation under `-Wall -Wextra -pedantic -std=c11`.
- [x] AddressSanitizer and UndefinedBehaviorSanitizer run clean on all suites.
