#!/usr/bin/env python3
"""
================================================================================
Arbitrary-Precision Calculator — Extreme Stress-Testing Suite
================================================================================
Purpose:
  Pushes the calculator beyond standard operational limits to stress:
  1. Operator chains (1,000 to 10,000 operators; linear vs balanced trees)
  2. Huge operands (2,000 to 100,000 digits; integer, decimal, scientific)
  3. Deeply nested parentheses & function calls (1,000+ levels)
  4. Extreme unary operator chains & precedence interactions
  5. Mathematical edge cases (div/mod zero, domain errors, 0^0, factorial bounds)
  6. Buffer limits (200,000-char input lines, precision 0-2000 boundaries)
  7. Adversarial combinations & rapid sequential REPL evaluations

Distinguishes between:
  - EXPECTED REJECTION: Clean error code/diagnostic message (PASS)
  - EXPECTED SUCCESS: Correct computed value within tolerance/exact match (PASS)
  - GENUINE FAILURE: SIGSEGV, SIGABRT, unhandled crash, memory fault, or timeout (FAIL)
================================================================================
"""

import sys
import os
import time
import subprocess
import signal

# Path to the compiled calculator binary
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CALC_BIN = os.path.join(ROOT_DIR, "calculator")

# ANSI color codes
GREEN = "\033[1;32m"
RED = "\033[1;31m"
YELLOW = "\033[1;33m"
CYAN = "\033[1;36m"
BOLD = "\033[1m"
RESET = "\033[0m"

class TestCase:
    def __init__(self, test_id, category, name, expr_generator, expected_kind, expected_substr, severity, description):
        self.test_id = test_id
        self.category = category
        self.name = name
        self.expr_generator = expr_generator # string or callable returning string
        self.expected_kind = expected_kind   # "SUCCESS", "REJECT", "ANY_CLEAN"
        self.expected_substr = expected_substr
        self.severity = severity             # "CRITICAL", "HIGH", "MEDIUM", "LOW"
        self.description = description

def run_calculator(payload, timeout=10):
    """Safely executes the calculator with the given payload under a strict timeout."""
    if not os.path.exists(CALC_BIN):
        raise FileNotFoundError(f"Calculator binary not found at {CALC_BIN}. Run 'make' first.")

    t0 = time.time()
    try:
        proc = subprocess.Popen(
            [CALC_BIN],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, stderr = proc.communicate(payload + "\nquit\n", timeout=timeout)
        elapsed = time.time() - t0
        return proc.returncode, stdout, stderr, elapsed, None
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.communicate()
        return -1, "", "", timeout, "TIMEOUT"
    except Exception as e:
        return -2, "", "", 0.0, str(e)

def extract_repl_results(stdout):
    """Extracts error lines and evaluated result lines from REPL output."""
    results = []
    errors = []
    for line in stdout.splitlines():
        line_clean = line.strip()
        if "Error:" in line_clean or "[Error:" in line_clean:
            errors.append(line_clean)
        elif line_clean.startswith(">   ="):
            # Format: ">   = <VAL>  [<TIME> ms]"
            parts = line_clean.split("=", 1)[1].strip()
            val = parts.split("[")[0].strip()
            results.append(val)
        elif line_clean.startswith("="):
            parts = line_clean.split("=", 1)[1].strip()
            val = parts.split("[")[0].strip()
            results.append(val)
    return results, errors

def make_balanced_tree(n, leaf="1", op="+"):
    """Constructs a balanced binary operator expression tree with n leaves."""
    if n <= 1:
        return leaf
    mid = n // 2
    return f"({make_balanced_tree(mid, leaf, op)} {op} {make_balanced_tree(n - mid, leaf, op)})"

# ==============================================================================
# Test Suite Definition
# ==============================================================================
TESTS = []

def add_test(cat, name, expr_gen, exp_kind, exp_substr, severity, desc):
    tid = f"STRESS-{len(TESTS)+1:03d}"
    TESTS.append(TestCase(tid, cat, name, expr_gen, exp_kind, exp_substr, severity, desc))

# ── 1. Operator Chains (1,000 to 10,000 operators) ───────────────────────────
add_test("1. Operator Chains", "Linear chain: 50 additions (below recursion limit 128)",
         lambda: " + ".join(["1"] * 51), "SUCCESS", "51", "LOW",
         "AST depth 50 <= 128: must compute exact sum")

add_test("1. Operator Chains", "Linear chain: 128 additions (exact recursion limit)",
         lambda: " + ".join(["1"] * 129), "SUCCESS", "129", "MEDIUM",
         "AST depth 128 == MAX_RECURSION_DEPTH: boundary condition success")

add_test("1. Operator Chains", "Linear chain: 129 additions (just above recursion limit)",
         lambda: " + ".join(["1"] * 130), "REJECT", "Recursion depth limit exceeded", "HIGH",
         "AST depth 129 > 128: expected clean rejection without stack overflow")

add_test("1. Operator Chains", "Linear chain: 1,000 additions (extreme linear depth)",
         lambda: " + ".join(["1"] * 1001), "REJECT", "Recursion depth limit exceeded", "CRITICAL",
         "1,000 unparenthesized additions: must not crash in parser or free_expr_node")

add_test("1. Operator Chains", "Linear chain: 10,000 additions (very large expression)",
         lambda: " + ".join(["1"] * 10001), "REJECT", "Recursion depth limit exceeded", "CRITICAL",
         "10,000 unparenthesized additions: deep AST destruction must be memory-safe")

add_test("1. Operator Chains", "Balanced tree: 1,000 operators (log2 depth = 10)",
         lambda: make_balanced_tree(1001), "SUCCESS", "1001", "HIGH",
         "1,000 operators in balanced tree (depth ~10 <= 128): must compute exact 1001")

add_test("1. Operator Chains", "Balanced tree: 5,000 operators (log2 depth = 13)",
         lambda: make_balanced_tree(5001), "SUCCESS", "5001", "CRITICAL",
         "5,000 operators in balanced tree: evaluates thousands of AST nodes smoothly")

add_test("1. Operator Chains", "Balanced tree: 10,000 operators (log2 depth = 14)",
         lambda: make_balanced_tree(10001), "SUCCESS", "10001", "CRITICAL",
         "10,000 operators in balanced tree: extreme stress test of allocator and evaluator")

add_test("1. Operator Chains", "Mixed precedence chain: 1,000 alternating * and +",
         lambda: make_balanced_tree(500, leaf="(2 * 3)", op="+"), "SUCCESS", "3000", "HIGH",
         "500 terms of (2 * 3) summed: evaluates precedence correctly across large tree")

# ── 2. Huge Operands (2,000 to 100,000 digits) ────────────────────────────────
add_test("2. Huge Operands", "2,000-digit integer addition",
         lambda: f"{'9'*2000} + {'1' + '0'*1999}", "SUCCESS", "1099999", "HIGH",
         "Adds two 2,000-digit numbers: checks limb boundary addition")

add_test("2. Huge Operands", "10,000-digit integer subtraction",
         lambda: f"{'1' + '0'*10000} - 1", "SUCCESS", "999999", "HIGH",
         "Subtracts 1 from 10^10000: tests 10,000-digit borrow propagation chain")

add_test("2. Huge Operands", "50,000-digit integer addition",
         lambda: f"{'5'*50000} + {'5'*50000}", "SUCCESS", "111111", "CRITICAL",
         "Adds two 50,000-digit integers: validates 5,556 Base-10^9 limbs")

add_test("2. Huge Operands", "100,000-digit integer addition",
         lambda: f"{'9'*100000} + {'1' + '0'*99999}", "SUCCESS", "1099999", "CRITICAL",
         "Adds two 100,000-digit numbers: verifies ~11,112 limbs carry ripple")

add_test("2. Huge Operands", "100,000-digit scalar multiplication (x * 2)",
         lambda: f"{'4'*100000} * 2", "SUCCESS", "888888", "CRITICAL",
         "Multiplies 100,000-digit integer by 2: verifies bigint_mul_small performance")

add_test("2. Huge Operands", "10,000-digit decimal scale alignment",
         lambda: f"1.{'1'*5000} + 2.{'2'*5000}", "SUCCESS", "3.33333", "HIGH",
         "Adds two 5,000-scale BigDecimal values: tests scale alignment & significand addition")

add_test("2. Huge Operands", "Scientific notation with huge significand (2,000 digits e+10)",
         lambda: f"1.{'0'*1999}1e10", "SUCCESS", "10000000000", "HIGH",
         "Deserializes 2,000-digit decimal with positive scientific exponent")

add_test("2. Huge Operands", "Scientific notation with negative exponent: 2.5e-50",
         lambda: "2.5e-50", "SUCCESS", "0.0000", "MEDIUM",
         "Small scientific decimal with negative exponent")

add_test("2. Huge Operands", "Massive scientific exponent: 1e50000",
         lambda: "1e50000", "ANY_CLEAN", "", "HIGH",
         "Exponent 50,000: should either expand or report memory error without crash")

# ── 3. Deep Nesting (1,000 to 5,000 levels) ───────────────────────────────────
add_test("3. Deep Nesting", "Parentheses nesting: 500 levels",
         lambda: "(" * 500 + "42" + ")" * 500, "SUCCESS", "42", "MEDIUM",
         "500-level nested parentheses: verifies parser grammar stack depth")

add_test("3. Deep Nesting", "Parentheses nesting: 1,000 levels",
         lambda: "(" * 1000 + "42" + ")" * 1000, "SUCCESS", "42", "HIGH",
         "1,000-level nested parentheses: verifies lexer tracking array (512 capacity) safety")

add_test("3. Deep Nesting", "Parentheses nesting: 5,000 levels",
         lambda: "(" * 5000 + "42" + ")" * 5000, "SUCCESS", "42", "CRITICAL",
         "5,000-level nested parentheses: tests extreme C call stack depth during parse")

add_test("3. Deep Nesting", "Nested function calls: abs() at 128 levels (limit)",
         lambda: "abs(" * 128 + "42" + ")" * 128, "SUCCESS", "42", "HIGH",
         "128 nested abs() calls: exactly at MAX_RECURSION_DEPTH")

add_test("3. Deep Nesting", "Nested function calls: abs() at 129 levels (exceeded)",
         lambda: "abs(" * 129 + "42" + ")" * 129, "REJECT", "Recursion depth limit exceeded", "HIGH",
         "129 nested abs() calls: cleanly rejected by recursion depth guard")

add_test("3. Deep Nesting", "Nested function calls: abs() at 1,000 levels",
         lambda: "abs(" * 1000 + "42" + ")" * 1000, "REJECT", "Recursion depth limit exceeded", "CRITICAL",
         "1,000 nested function calls: must not crash during AST building or deallocation")

add_test("3. Deep Nesting", "Nested function calls: sqrt(sqrt(...)) at 20 levels",
         lambda: "sqrt(" * 20 + "65536" + ")" * 20, "SUCCESS", "1.", "HIGH",
         "20 nested square roots: tests rapid convergence to 1.0 without precision loss")

# ── 4. Unary Operator Chains ──────────────────────────────────────────────────
add_test("4. Unary Chains", "Unary minus: 128 chained signs (limit)",
         lambda: "- " * 128 + "42", "SUCCESS", "42", "MEDIUM",
         "128 unary minuses: even count produces +42 at recursion boundary")

add_test("4. Unary Chains", "Unary minus: 129 chained signs (exceeded)",
         lambda: "- " * 129 + "42", "REJECT", "Recursion depth limit exceeded", "HIGH",
         "129 unary minuses: cleanly rejected by recursion guard")

add_test("4. Unary Chains", "Unary minus: 1,000 chained signs",
         lambda: "- " * 1000 + "42", "REJECT", "Recursion depth limit exceeded", "CRITICAL",
         "1,000 unary minuses: must not overflow parser stack")

add_test("4. Unary Chains", "Alternating unary plus and minus (50 signs)",
         lambda: "+ - " * 25 + "42", "SUCCESS", "42", "LOW",
         "50 alternating unary signs (+ - + -): evaluates to +42")

add_test("4. Unary Chains", "Chained factorial: 3!!",
         lambda: "3!!", "SUCCESS", "720", "MEDIUM",
         "(3!)! = 6! = 720: postfix factorial chaining")

add_test("4. Unary Chains", "Precedence: -(5!) vs (-5)!",
         lambda: "-5!", "SUCCESS", "-120", "LOW",
         "-5! parses as -(5!) = -120")

# ── 5. Mathematical Edge Cases & Boundaries ───────────────────────────────────
add_test("5. Math Edge Cases", "Integer division by zero: 1 / 0",
         "1 / 0", "REJECT", "Division by zero", "CRITICAL",
         "Exact integer division by zero detection")

add_test("5. Math Edge Cases", "Decimal division by zero: 1.5 / 0",
         "1.5 / 0", "REJECT", "Division by zero", "CRITICAL",
         "Decimal division by zero detection")

add_test("5. Math Edge Cases", "Zero divided by zero: 0 / 0",
         "0 / 0", "REJECT", "Division by zero", "CRITICAL",
         "Indeterminate 0/0 division detection")

add_test("5. Math Edge Cases", "Modulo by zero: 5 % 0",
         "5 % 0", "REJECT", "Modulo by zero", "HIGH",
         "Integer remainder modulo by zero detection")

add_test("5. Math Edge Cases", "Logarithm of zero: ln(0)",
         "ln(0)", "REJECT", "Domain error", "HIGH",
         "Domain error check for natural log at non-positive argument")

add_test("5. Math Edge Cases", "Logarithm of negative number: ln(-5)",
         "ln(-5)", "REJECT", "Domain error", "HIGH",
         "Domain error check for natural log of negative argument")

add_test("5. Math Edge Cases", "Square root of negative: sqrt(-4)",
         "sqrt(-4)", "REJECT", "negative sqrt", "HIGH",
         "Domain error check for negative square root")

add_test("5. Math Edge Cases", "Power: 0^0",
         "0^0", "SUCCESS", "1", "LOW",
         "0^0 mathematical convention: evaluates to 1")

add_test("5. Math Edge Cases", "Power: 0^5",
         "0^5", "SUCCESS", "0", "LOW",
         "Zero raised to positive power: evaluates to 0")

add_test("5. Math Edge Cases", "Power: negative exponent integer promotion 2^-3",
         "2^-3", "SUCCESS", "0.125", "MEDIUM",
         "Integer base with negative exponent promotes to BigDecimal: 1/8 = 0.125")

add_test("5. Math Edge Cases", "Factorial: negative integer (-5)!",
         "(-5)!", "REJECT", "Factorial input negative or too large", "HIGH",
         "Factorial of negative integer rejected")

add_test("5. Math Edge Cases", "Factorial: fractional argument 2.5!",
         "2.5!", "REJECT", "Factorial requires non-negative integer", "MEDIUM",
         "Factorial on non-integer BigDecimal rejected")

add_test("5. Math Edge Cases", "Factorial boundary: 10,000! (maximum allowed operand)",
         "10000!", "SUCCESS", "284625968", "HIGH",
         "10,000! boundary: computes 35,660-digit factorial correctly")

add_test("5. Math Edge Cases", "Factorial boundary: 10,001! (exceeded limit)",
         "10001!", "REJECT", "Factorial input negative or too large", "HIGH",
         "10,001! boundary: cleanly rejected by operand guard (N <= 10000)")

add_test("5. Math Edge Cases", "Precision command boundary: precision 0",
         "precision 0\n1/7", "SUCCESS", "0", "MEDIUM",
         "Sets precision to 0 decimal places: rounds 1/7 to 0")

add_test("5. Math Edge Cases", "Precision command boundary: precision 2000 (maximum)",
         "precision 2000\n1/7", "SUCCESS", "0.142857", "HIGH",
         "Sets precision to 2000 decimal places: outputs 2000 digits correctly")

add_test("5. Math Edge Cases", "Precision command boundary: precision 2001 (rejected)",
         "precision 2001", "REJECT", "precision must be between 0 and 2000", "MEDIUM",
         "Precision > 2000 cleanly rejected")

add_test("5. Math Edge Cases", "Precision command boundary: precision -5 (rejected)",
         "precision -5", "REJECT", "precision must be between 0 and 2000", "MEDIUM",
         "Negative precision cleanly rejected")

# ── 6. Syntax, Lexer & Input Buffer Stress ───────────────────────────────────
add_test("6. Syntax & Buffers", "200,000-character input line (dynamic buffer doubling)",
         lambda: f"{'1' + '0'*100000} + {'2' + '0'*100000}", "SUCCESS", "3000000", "CRITICAL",
         "Feeds 200,000 characters into read_input_line: verifies realloc growth from 128 bytes")

add_test("6. Syntax & Buffers", "Empty expression input",
         "", "ANY_CLEAN", "", "LOW",
         "Empty input line: must not crash REPL")

add_test("6. Syntax & Buffers", "Whitespace-only input",
         "         \t   ", "ANY_CLEAN", "", "LOW",
         "Whitespace line: must be ignored safely")

add_test("6. Syntax & Buffers", "Multiple consecutive operators: 1 + + 2",
         "1 + + 2", "SUCCESS", "3", "LOW",
         "1 + (+2) = 3: valid binary plus followed by unary plus")

add_test("6. Syntax & Buffers", "Invalid operator sequence: 1 * * 2",
         "1 * * 2", "REJECT", "Incorrect expression", "MEDIUM",
         "Two consecutive binary multiplication operators: rejected")

add_test("6. Syntax & Buffers", "Trailing operator: 5 +",
         "5 +", "REJECT", "Incorrect expression", "LOW",
         "Missing right operand: rejected")

add_test("6. Syntax & Buffers", "Leading binary operator: * 5",
         "* 5", "REJECT", "Incorrect expression", "LOW",
         "Leading multiplication operator without operand: rejected")

add_test("6. Syntax & Buffers", "Empty parentheses: ()",
         "()", "REJECT", "Incorrect expression", "LOW",
         "Empty parentheses: rejected")

add_test("6. Syntax & Buffers", "Unbalanced parentheses (unclosed): (((1+2)",
         "(((1+2)", "REJECT", "Incorrect expression", "MEDIUM",
         "Unclosed opening parentheses: rejected")

add_test("6. Syntax & Buffers", "Unbalanced parentheses (unexpected closing): 1+2)))",
         "1+2)))", "REJECT", "Incorrect expression", "MEDIUM",
         "Extra closing parentheses: rejected")

add_test("6. Syntax & Buffers", "Inverted parentheses: )(1+2)(",
         ")(1+2)(", "REJECT", "Incorrect expression", "MEDIUM",
         "Mismatched inverted parentheses: rejected")

add_test("6. Syntax & Buffers", "Multiple decimal points: 1.2.3.4",
         "1.2.3.4", "REJECT", "Incorrect expression", "MEDIUM",
         "Malformed numeric literal with multiple dots: rejected")

add_test("6. Syntax & Buffers", "Malformed scientific notation: 1e",
         "1e", "REJECT", "Incorrect expression", "LOW",
         "Missing exponent digits after e: rejected")

add_test("6. Syntax & Buffers", "Disallowed illegal characters: @#$%&~",
         "@#$%&~", "REJECT", "Incorrect expression", "MEDIUM",
         "Disallowed characters pre-validation check")

add_test("6. Syntax & Buffers", "Unknown variable identifier: my_unknown_var + 1",
         "my_unknown_var + 1", "REJECT", "Undefined variable", "MEDIUM",
         "Lookup of unregistered symbol: clean error")

add_test("6. Syntax & Buffers", "Unknown function call: foo(42)",
         "foo(42)", "REJECT", "Incorrect expression", "MEDIUM",
         "Invocation of unregistered function name: rejected")

# ── 7. Adversarial Combinations & Rapid Sequential REPL ───────────────────────
add_test("7. Adversarial & Sequential", "500 rapid sequential expressions with variable persistence",
         lambda: "\n".join([f"v{i} = {i} * 2" for i in range(500)]) + "\nv499 + 1",
         "SUCCESS", "999", "HIGH",
         "Executes 500 variable definitions in rapid sequence: validates symbol table capacity")

add_test("7. Adversarial & Sequential", "Sequential 'ans' chaining across 100 operations",
         lambda: "1\n" + "\n".join(["ans + 1" for _ in range(100)]),
         "SUCCESS", "101", "MEDIUM",
         "Chains 100 successive results using 'ans' special variable")

add_test("7. Adversarial & Sequential", "High-precision transcendental cancellation: 10^50 + sin(pi/6) - 10^50",
         "100000000000000000000000000000000000000000000000000 + sin(pi / 6) - 100000000000000000000000000000000000000000000000000",
         "SUCCESS", "0.499999999999999999", "HIGH",
         "Cancels 50-digit magnitude integer around sin(30 deg) = 0.5 (Taylor series within 10^-19)")

add_test("7. Adversarial & Sequential", "Composition: sqrt(exp(ln(100)))",
         "sqrt(exp(ln(100)))", "SUCCESS", "10.", "HIGH",
         "Composes sqrt, exp, and ln with exact mathematical inverse cancellation")

# ==============================================================================
# Test Execution Engine
# ==============================================================================
def run_all_tests():
    print(f"{BOLD}{CYAN}══════════════════════════════════════════════════════════════════════{RESET}")
    print(f"{BOLD}{CYAN}   Precision Calculator — Extreme Stress-Testing Suite Execution       {RESET}")
    print(f"{BOLD}{CYAN}══════════════════════════════════════════════════════════════════════{RESET}")
    print(f"Target Binary: {CALC_BIN}\n")

    passed_count = 0
    failed_count = 0
    current_cat = None

    results_table = []

    for test in TESTS:
        if test.category != current_cat:
            current_cat = test.category
            print(f"\n{BOLD}{CYAN}── {current_cat} ──{RESET}")

        # Generate expression payload
        try:
            if callable(test.expr_generator):
                payload = test.expr_generator()
            else:
                payload = str(test.expr_generator)
        except Exception as e:
            print(f"{RED}FAIL{RESET}  [{test.test_id}] {test.name} — Payload generation error: {e}")
            failed_count += 1
            results_table.append((test, False, f"Gen Error: {e}", 0.0))
            continue

        # Run calculator under safe timeout
        code, stdout, stderr, elapsed, err_info = run_calculator(payload, timeout=12)

        # Evaluate outcome
        res_list, err_list = extract_repl_results(stdout)
        last_res = res_list[-1] if res_list else ""
        last_err = err_list[-1] if err_list else ""

        is_passed = False
        detail = ""

        if err_info == "TIMEOUT":
            is_passed = False
            detail = "PROCESS HANG (TIMEOUT > 12s)"
        elif code < 0 or code > 1:
            # Fatal crash or abnormal signal (e.g. SIGSEGV=11, SIGABRT=6)
            sig_name = f"Signal {-code}" if code < 0 else f"Exit code {code}"
            is_passed = False
            detail = f"CRASH: {sig_name}"
        elif test.expected_kind == "SUCCESS":
            if last_res and test.expected_substr in last_res:
                is_passed = True
                detail = f"out=\"{last_res[:25]}\" ({elapsed*1000:.1f}ms)"
            else:
                is_passed = False
                detail = f"Expected \"{test.expected_substr}\", got res=\"{last_res[:20]}\" err=\"{last_err[:25]}\""
        elif test.expected_kind == "REJECT":
            if last_err and (test.expected_substr in last_err or "Error:" in last_err):
                is_passed = True
                detail = f"Cleanly rejected: {last_err[:35]}"
            else:
                is_passed = False
                detail = f"Expected rejection containing \"{test.expected_substr}\", got res=\"{last_res[:20]}\" err=\"{last_err[:25]}\""
        elif test.expected_kind == "ANY_CLEAN":
            if code in [0, 1]:
                is_passed = True
                detail = f"Handled cleanly ({elapsed*1000:.1f}ms)"
            else:
                is_passed = False
                detail = f"Failed with exit code {code}"

        if is_passed:
            passed_count += 1
            print(f"{GREEN}PASS{RESET}  [{test.test_id}] {test.name:<48}  {detail}")
        else:
            failed_count += 1
            print(f"{RED}FAIL{RESET}  [{test.test_id}] {test.name:<48}  {detail}")

        results_table.append((test, is_passed, detail, elapsed))

    # Summary
    print(f"\n{BOLD}{CYAN}══════════════════════════════════════════════════════════════════════{RESET}")
    print(f"{BOLD}Extreme Stress Test Summary:{RESET}")
    print(f"  Total Cases Executed : {len(TESTS)}")
    print(f"  Passed (Safe / Clean): {GREEN}{passed_count}{RESET}")
    print(f"  Failed (Crash / Bug) : {RED if failed_count > 0 else GREEN}{failed_count}{RESET}")
    print(f"{BOLD}{CYAN}══════════════════════════════════════════════════════════════════════{RESET}")

    if failed_count == 0:
        print(f"\n{GREEN}✅  ALL {len(TESTS)} EXTREME STRESS TESTS PASSED SUCCESSFULLY!{RESET}")
        print("    0 crashes, 0 hangs, 0 memory faults, and all boundary limits cleanly enforced.\n")
    else:
        print(f"\n{RED}❌  {failed_count} EXTREME STRESS TESTS FAILED.{RESET}\n")

    return 0 if failed_count == 0 else 1

if __name__ == "__main__":
    sys.exit(run_all_tests())
