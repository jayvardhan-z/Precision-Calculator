# Manual Quality Assurance & Usability Checklist

**Project:** Precision-Calculator  
**Standard:** ISO/IEC 25010 Usability & Human Factors  
**Classification:** Manual Testing Protocol & Human Interface Checklist  

This checklist provides manual verification protocols for human testers to validate the interactive Read-Eval-Print Loop (REPL), formatting, prompt appearance, and user experience.

---

## 1. Interactive REPL Look-and-Feel Protocol

| Item ID | Test Action | Verification Criterion | Expected Visual Outcome | Priority | Tester Status | Comments |
| :---: | :--- | :--- | :--- | :---: | :---: | :--- |
| **MAN-01** | Launch `./calculator` without arguments | Startup banner renders with proper spacing | Displays:<br>`=== Arbitrary-Precision Calculator ===`<br>`Type 'help' for commands, 'exit' or Ctrl+D to quit.`<br>followed by an empty line and `> ` prompt. | High | [ ] PASS<br>[ ] FAIL | Confirm no ANSI escape garbage |
| **MAN-02** | Observe prompt glyph | Interactive input prompt | Prompt is exactly `> ` without leading spaces. | Medium | [ ] PASS<br>[ ] FAIL | Standard REPL convention |
| **MAN-03** | Execute `help` command | Command and syntax cheat sheet | Displays clean tabular layout with Commands (`exit`, `help`, `mode`, `precision`) and Syntax (`+ - * / % ^ !`, `sin cos exp ln`, `sqrt abs gcd`, `pi, e, ans`). | High | [ ] PASS<br>[ ] FAIL | Ensure variable assignment (`x = expr`) is NOT present |
| **MAN-04** | Execute `?` | Short alias for help | Exact identical output to `help`. | Low | [ ] PASS<br>[ ] FAIL | Usability convenience |
| **MAN-05** | Evaluate `2 + 2` | Output formatting and timing | Displays `  = 4  [0.00XX ms]` followed by an empty line and new `> ` prompt. | High | [ ] PASS<br>[ ] FAIL | Check indentation (2 spaces) |
| **MAN-06** | Type empty line (Enter key) | Input tolerance | Silent ignore; re-displays `> ` without crashing or printing error. | Medium | [ ] PASS<br>[ ] FAIL | Rulebook §14 compliance |
| **MAN-07** | Input `2 * (3 + 4` (unclosed paren)| Error visual diagnosis | Output begins with `  Error: ` describing syntax issue cleanly. | High | [ ] PASS<br>[ ] FAIL | Clear human readability |
| **MAN-08** | Change angle mode via `mode` | Mode toggle notification | Displays `  [Angle mode is now DEGREE]` or `  [Angle mode is now RADIAN]`. | High | [ ] PASS<br>[ ] FAIL | State visibility |
| **MAN-09** | Update precision via `precision 50` | Precision notification | Displays `  [Precision set to 50 decimal places]`. | High | [ ] PASS<br>[ ] FAIL | Immediate feedback |
| **MAN-10** | Attempt invalid precision `precision -5` | Constraint guard | Displays `  [Error: precision must be between 0 and 2000]`. | Medium | [ ] PASS<br>[ ] FAIL | Boundary enforcement |
| **MAN-11** | Press `Ctrl+D` (EOF) at prompt | Clean session termination | Prints `Exiting.` and exits to shell with code `0`. | High | [ ] PASS<br>[ ] FAIL | Clean POSIX EOF exit |
| **MAN-12** | Execute `exit` or `quit` or `q` | Clean session termination | Immediately exits to shell with code `0`. | High | [ ] PASS<br>[ ] FAIL | Multiple exit aliases |
| **MAN-13** | Long digit output display | Terminal wrapping | Numbers with 1,000+ digits display without truncation or intermediate newlines. | Medium | [ ] PASS<br>[ ] FAIL | Unbounded limb rendering |

---

## 2. Usability & User Experience Review Notes
- **Clarity of Error Messages:** All error messages start with `  Error: ` and state the mathematical or syntactic cause directly without leaking internal C pointers or abort codes.
- **Latency Readability:** The execution latency bracket `[X.XXXX ms]` provides immediate visual confirmation of execution speed.

