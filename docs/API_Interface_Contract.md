# API Interface Contract & Specification

**Project Name:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Project Identifier:** Precision-Calculator  
**Version:** 2.0.0  
**Specification Type:** Dual-Tier Contract (C Native API & OpenAPI 3.1.0 REST Specification)  

---

## 1. Native C Library Interface Contract (`libprecisioncalc`)

This section specifies the C11 binary interface for embedders wishing to link directly against `libprecisioncalc`.

### 1.1 Type Definitions & Memory Ownership
* **Memory Invariant:** Functions taking pointer-to-pointer or destination pointers expect the caller to pass initialized or uninitialized targets as specified. Output values returned by value or pointer become the caller's responsibility to release using the corresponding `*_free()` lifecycle function.
* **Thread Safety:** The core arithmetic and evaluation functions (`eval_node`, `decimal_add`, `bigint_mul`, etc.) are fully reentrant and thread-safe when invoked with independent `EvalContext` and `SymbolTable` instances.

### 1.2 Core Public API Functions

#### `eval_node`
```c
int eval_node(const ExprNode *n, EvalContext *ctx, Value *out);
```
* **Description:** Recursively evaluates an AST node `n` within evaluation context `ctx` and writes the result into `out`.
* **Parameters:**
  * `n` `[const ExprNode*]`: Pointer to root or branch AST node.
  * `ctx` `[EvalContext*]`: Runtime context holding precision, angle mode, and symbol table.
  * `out` `[Value*]`: Target `Value` container (pre-cleared or uninitialized).
* **Returns:** `1` on success, `0` on evaluation error (details stored in `ctx->error`).

#### `symtab_set` / `symtab_get`
```c
int symtab_set(SymbolTable *st, const char *name, const Value *v);
int symtab_get(const SymbolTable *st, const char *name, Value *out);
```
* **Description:** Mutates or queries variable bindings in a symbol table.
* **Parameters:**
  * `st` `[SymbolTable*]`: Target symbol table.
  * `name` `[const char*]`: Null-terminated ASCII identifier name.
  * `v` / `out` `[Value*]`: Value to store (deep-copied) or target container to populate.
* **Returns:** `1` if inserted/found, `0` on error/missing.

#### `value_to_string`
```c
char *value_to_string(const Value *v);
```
* **Description:** Losslessly renders any `Value` (integer, decimal, complex) into a newly allocated C-string.
* **Returns:** Heap-allocated null-terminated ASCII string. Caller must invoke `free()`.

---

## 2. CLI Interface Contract

### 2.1 Command-Line Invocations

#### Interactive Mode (REPL)
```bash
./calculator
```
* Launches the interactive Read-Eval-Print Loop on `stdin`.

#### Batch Expression Mode
```bash
./calculator "2^100 + 5"
```
* Evaluates the expression directly, prints the formatted result and execution time, and terminates with exit code `0`.

#### Piped / Headless Execution
```bash
echo "sin(pi / 4)" | ./calculator
```

### 2.2 CLI Exit Status Codes
| Exit Code | Meaning |
| :---: | :--- |
| `0` | Success: Expression evaluated and printed cleanly. |
| `1` | Syntax / Evaluation Error: Invalid expression, division by zero, or domain violation. |
| `2` | System / Allocation Error: Out-of-memory or OS resource exhaustion. |

### 2.3 REPL Meta-Commands
* `:mode <rad|deg>`: Toggles angular interpretation for trigonometric functions.
* `:precision <N>`: Sets fractional decimal evaluation precision (default: 20).
* `:vars`: Displays all currently bound variables and preloaded constants.
* `:help`: Prints interactive command usage instructions.
* `:exit` or `:quit`: Gracefully terminates the REPL.

---

## 3. OpenAPI 3.1.0 REST Service Contract

When Precision-Calculator is wrapped as a microservice daemon (e.g. for Web, Mobile, or Cloud RPCs), the following OpenAPI 3.1.0 specification defines the HTTP contract.

```yaml
openapi: 3.1.0
info:
  title: Precision-Calculator Cloud Microservice API
  description: High-performance arbitrary-precision calculation engine REST service.
  version: 2.0.0
  contact:
    name: Precision-Calculator Engineering Team
    url: https://github.com/jayvardhan-z/Precision-Calculator

servers:
  - url: https://api.calculator.internal/v1
    description: Internal Production Microservice Gateway
  - url: http://localhost:8080/v1
    description: Local Development Daemon

security:
  - BearerAuth: []

paths:
  /eval:
    post:
      summary: Evaluate Mathematical Expression
      description: Parses and computes an infix mathematical expression with arbitrary precision.
      operationId: evaluateExpression
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/EvaluationRequest'
            examples:
              basicArithmetic:
                summary: Integer and decimal calculation
                value:
                  expression: "2^64 + 10 / 4"
                  precision: 20
                  angle_mode: "radian"
              complexArithmetic:
                summary: Complex number multiplication
                value:
                  expression: "(2 + 3*i) * (4 + 5*i)"
                  precision: 20
      responses:
        '200':
          description: Calculation completed successfully.
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/EvaluationResponse'
        '400':
          description: Syntax, domain, or arithmetic evaluation error.
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/ErrorResponse'
        '401':
          description: Unauthorized: Missing or invalid JWT token.
        '422':
          description: Unprocessable request: Recursion depth or memory limit exceeded.
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/ErrorResponse'

  /constants:
    get:
      summary: Retrieve Mathematical Constants
      description: Returns precomputed mathematical constants evaluated to the requested precision.
      operationId: getConstants
      parameters:
        - name: precision
          in: query
          required: false
          schema:
            type: integer
            default: 50
            minimum: 1
            maximum: 120
      responses:
        '200':
          description: Dictionary of high-precision constants.
          content:
            application/json:
              schema:
                type: object
                properties:
                  pi:
                    type: string
                    example: "3.1415926535897932384626433832795028841971693993751"
                  e:
                    type: string
                    example: "2.7182818284590452353602874713526624977572470936999"
                  ln10:
                    type: string
                    example: "2.3025850929940456840179914546843642076011014886287"

components:
  securitySchemes:
    BearerAuth:
      type: http
      scheme: bearer
      bearerFormat: JWT

  schemas:
    EvaluationRequest:
      type: object
      required:
        - expression
      properties:
        expression:
          type: string
          description: Infix mathematical expression string.
          example: "sin(pi / 4) + sqrt(144)"
        precision:
          type: integer
          description: Target decimal digits for division and scientific functions.
          default: 20
          minimum: 0
          maximum: 1000
        angle_mode:
          type: string
          enum: [radian, degree]
          default: radian
        variables:
          type: object
          additionalProperties:
            type: string
          description: Key-value map of variable names to pre-bound values.
          example:
            x: "1000000000000"
            y: "3.14159"

    EvaluationResponse:
      type: object
      required:
        - status
        - result
        - type
        - elapsed_ms
      properties:
        status:
          type: string
          example: "success"
        result:
          type: string
          description: Exact or high-precision formatted string result.
          example: "12.70710678118654752440"
        type:
          type: string
          enum: [integer, decimal, complex]
          example: "decimal"
        elapsed_ms:
          type: number
          format: double
          description: Monotonic arithmetic evaluation latency in milliseconds.
          example: 0.048

    ErrorResponse:
      type: object
      required:
        - status
        - error_code
        - message
        - position
      properties:
        status:
          type: string
          example: "error"
        error_code:
          type: string
          enum:
            - ERR_SYNTAX
            - ERR_DIV_BY_ZERO
            - ERR_UNDEFINED_VAR
            - ERR_UNDEFINED_FUNC
            - ERR_INVALID_ARITY
            - ERR_TYPE_MISMATCH
            - ERR_DOMAIN
            - ERR_RECURSION_LIMIT
            - ERR_OVERFLOW
            - ERR_INTERNAL
          example: "ERR_DIV_BY_ZERO"
        message:
          type: string
          example: "Division by zero"
        position:
          type: integer
          description: Character byte offset in input expression where failure was detected.
          example: 9
```
