#ifndef REPL_H
#define REPL_H

// ============================================================================
// File: repl.h
// Purpose: Interactive Read-Eval-Print Loop (REPL) and CLI runner interface.
// Role:
//   Provides the user-facing terminal interface, command processor,
//   expression executor, and variable environment manager.
// ============================================================================

// Prints the welcoming banner, command cheat-sheet, and exit instructions
void display_menu(void);

// Launches the interactive prompt loop reading expressions and commands from stdin
// Returns 0 on clean exit
int run_repl(void);

// Evaluates a single mathematical expression string, prints the result, and returns status
// Returns 0 on success, non-zero on evaluation or syntax error
int run_expression(const char *expr);

#endif /* REPL_H */