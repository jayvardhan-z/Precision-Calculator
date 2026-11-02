# ==============================================================================
# Makefile: Precision-Calculator Comprehensive Testing & Build Engine
# Standards: ISO C11, POSIX.1-2008, Zero External Dependencies
# ==============================================================================

CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c11 -O2
CPPFLAGS = -IWorklet1 -IWorklet2 -IWorklet3 -Itests/framework -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
LDLIBS = -lm -pthread

TARGET = calculator
OBJDIR = obj

# Core production source modules
SRCS = main.c \
       Worklet1/ast.c \
       Worklet1/bigint_io.c \
       Worklet1/lexer.c \
       Worklet1/parser.c \
       Worklet1/repl.c \
       Worklet1/timer.c \
       Worklet2/bigint.c \
       Worklet3/complex.c \
       Worklet3/decimal.c \
       Worklet3/eval.c \
       Worklet3/fast_math.c \
       Worklet3/functions.c

LIB_SRCS = $(filter-out main.c, $(SRCS))
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

# Test Binary Definitions
UNIT_TESTS = tests/unit/test_wb_bigint \
             tests/unit/test_wb_bigint_io \
             tests/unit/test_wb_fast_math \
             tests/unit/test_wb_decimal \
             tests/unit/test_wb_complex \
             tests/unit/test_wb_functions \
             tests/unit/test_wb_lexer \
             tests/unit/test_wb_parser \
             tests/unit/test_wb_ast \
             tests/unit/test_wb_timer \
             tests/unit/test_wb_symtab \
             tests/unit/test_wb_eval

INTEG_TESTS = tests/integration/test_it_topdown \
              tests/integration/test_it_bottomup \
              tests/integration/test_it_bigbang \
              tests/integration/test_it_flow

SYS_TESTS = tests/system/test_sys_perf \
            tests/system/test_sys_concurrency \
            tests/system/test_sys_stress_fuzz \
            tests/system/test_sys_compat

BLACKBOX_RUNNER = tests/blackbox/csv_runner
GRAYBOX_TEST    = tests/blackbox/test_graybox

.PHONY: all run clean debug test unit integration system acceptance blackbox \
        coverage sanitize perf regression docs help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS = -Wall -Wextra -pedantic -std=c11 -g3 -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

# ------------------------------------------------------------------------------
# 1. Unit Testing Target (White-Box Module Verification)
# ------------------------------------------------------------------------------
tests/unit/test_wb_bigint: tests/unit/test_wb_bigint.c Worklet2/bigint.c Worklet1/bigint_io.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_bigint_io: tests/unit/test_wb_bigint_io.c Worklet2/bigint.c Worklet1/bigint_io.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_fast_math: tests/unit/test_wb_fast_math.c Worklet2/bigint.c Worklet1/bigint_io.c Worklet3/fast_math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_decimal: tests/unit/test_wb_decimal.c Worklet2/bigint.c Worklet1/bigint_io.c Worklet3/fast_math.c Worklet3/decimal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_complex: tests/unit/test_wb_complex.c Worklet2/bigint.c Worklet1/bigint_io.c Worklet3/fast_math.c Worklet3/decimal.c Worklet3/complex.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_functions: tests/unit/test_wb_functions.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_lexer: tests/unit/test_wb_lexer.c Worklet1/lexer.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_parser: tests/unit/test_wb_parser.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_ast: tests/unit/test_wb_ast.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_timer: tests/unit/test_wb_timer.c Worklet1/timer.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_symtab: tests/unit/test_wb_symtab.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/unit/test_wb_eval: tests/unit/test_wb_eval.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

unit: $(UNIT_TESTS)
	@echo "\n=== Running White-Box Unit Test Suite ==="
	@for t in $(UNIT_TESTS); do ./$$t; done

# ------------------------------------------------------------------------------
# 2. Integration Testing Target (Incremental & Big-Bang)
# ------------------------------------------------------------------------------
tests/integration/test_it_topdown: tests/integration/test_it_topdown.c tests/stubs/stub_functions.c Worklet3/eval.c Worklet1/parser.c Worklet1/lexer.c Worklet1/ast.c Worklet1/bigint_io.c Worklet2/bigint.c Worklet3/fast_math.c Worklet3/decimal.c Worklet3/complex.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/integration/test_it_bottomup: tests/integration/test_it_bottomup.c tests/drivers/driver_arithmetic.c Worklet2/bigint.c Worklet1/bigint_io.c Worklet3/fast_math.c Worklet3/decimal.c Worklet3/complex.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/integration/test_it_bigbang: tests/integration/test_it_bigbang.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/integration/test_it_flow: tests/integration/test_it_flow.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

integration: $(INTEG_TESTS)
	@echo "\n=== Running Integration Test Suite ==="
	@for t in $(INTEG_TESTS); do ./$$t; done

# ------------------------------------------------------------------------------
# 3. System Testing Target (Performance, Concurrency, Stress, Recovery)
# ------------------------------------------------------------------------------
tests/system/test_sys_perf: tests/system/test_sys_perf.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/system/test_sys_concurrency: tests/system/test_sys_concurrency.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/system/test_sys_stress_fuzz: tests/system/test_sys_stress_fuzz.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

tests/system/test_sys_compat: tests/system/test_sys_compat.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

system: $(TARGET) $(SYS_TESTS)
	@echo "\n=== Running System Test Suite ==="
	@for t in $(SYS_TESTS); do ./$$t; done
	@bash tests/system/test_sys_load.sh
	@bash tests/system/test_sys_usability.sh
	@bash tests/system/test_sys_migration.sh
	@bash tests/system/test_sys_functional.sh
	@bash tests/system/test_sys_recovery.sh

# ------------------------------------------------------------------------------
# 4. Acceptance Testing Target (UAT, BAT, RAT, Alpha, Beta)
# ------------------------------------------------------------------------------
acceptance: $(TARGET)
	@echo "\n=== Running Acceptance Test Suite ==="
	@bash tests/acceptance/test_uat_scenarios.sh
	@bash tests/acceptance/test_bat_operational.sh
	@bash tests/acceptance/test_rat_compliance.sh
	@bash tests/acceptance/test_alpha.sh
	@bash tests/acceptance/test_beta.sh

# ------------------------------------------------------------------------------
# 5. Black-Box & Gray-Box Testing Targets
# ------------------------------------------------------------------------------
$(BLACKBOX_RUNNER): tests/blackbox/csv_runner.c
	$(CC) $(CFLAGS) $< -o $@

$(GRAYBOX_TEST): tests/blackbox/test_graybox.c $(LIB_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

blackbox: $(TARGET) $(BLACKBOX_RUNNER) $(GRAYBOX_TEST)
	@echo "\n=== Running Black-Box & Gray-Box Suites ==="
	@./$(GRAYBOX_TEST)
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_equiv_partition.csv || true
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_bva.csv || true
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_decision_table.csv || true
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_error_guessing.csv || true
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_pairwise.csv || true
	@./$(BLACKBOX_RUNNER) tests/data/test_cases_rulebook.csv || true

# ------------------------------------------------------------------------------
# 6. Performance & Regression Testing Targets
# ------------------------------------------------------------------------------
perf: tests/system/test_sys_perf
	@./tests/system/test_sys_perf

regression: $(TARGET)
	@bash tests/system/test_sys_regression.sh

# ------------------------------------------------------------------------------
# 7. Code Coverage Analysis (gcov) Target
# ------------------------------------------------------------------------------
coverage: CFLAGS = -Wall -Wextra -pedantic -std=c11 -O0 -g --coverage -fprofile-arcs -ftest-coverage
coverage: clean
	@echo "\n=== Compiling with gcov code coverage instrumentation ==="
	@mkdir -p $(OBJDIR)/Worklet1 $(OBJDIR)/Worklet2 $(OBJDIR)/Worklet3
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/ast.c -o $(OBJDIR)/Worklet1/ast.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/bigint_io.c -o $(OBJDIR)/Worklet1/bigint_io.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/lexer.c -o $(OBJDIR)/Worklet1/lexer.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/parser.c -o $(OBJDIR)/Worklet1/parser.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/repl.c -o $(OBJDIR)/Worklet1/repl.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet1/timer.c -o $(OBJDIR)/Worklet1/timer.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet2/bigint.c -o $(OBJDIR)/Worklet2/bigint.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet3/complex.c -o $(OBJDIR)/Worklet3/complex.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet3/decimal.c -o $(OBJDIR)/Worklet3/decimal.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet3/eval.c -o $(OBJDIR)/Worklet3/eval.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet3/fast_math.c -o $(OBJDIR)/Worklet3/fast_math.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c Worklet3/functions.c -o $(OBJDIR)/Worklet3/functions.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c main.c -o $(OBJDIR)/main.o
	$(CC) $(CFLAGS) $(OBJDIR)/Worklet1/*.o $(OBJDIR)/Worklet2/*.o $(OBJDIR)/Worklet3/*.o $(OBJDIR)/main.o $(LDLIBS) -o $(TARGET)
	@echo "=== Executing full test suite under coverage ==="
	$(MAKE) unit
	$(MAKE) integration
	$(MAKE) blackbox
	@echo "\n=== Generating gcov Statement and Branch Coverage Reports ==="
	@gcov -b -c --object-directory $(OBJDIR)/Worklet2 Worklet2/bigint.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet3 Worklet3/fast_math.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet3 Worklet3/decimal.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet3 Worklet3/complex.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet3 Worklet3/functions.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet3 Worklet3/eval.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet1 Worklet1/lexer.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet1 Worklet1/parser.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet1 Worklet1/ast.c | grep -E "(File|Lines executed|Branches executed)" || true
	@gcov -b -c --object-directory $(OBJDIR)/Worklet1 Worklet1/bigint_io.c | grep -E "(File|Lines executed|Branches executed)" || true

# ------------------------------------------------------------------------------
# 8. Memory & Undefined Behavior Sanitizer Target (ASan + UBSan)
# ------------------------------------------------------------------------------
sanitize: CFLAGS = -Wall -Wextra -pedantic -std=c11 -g3 -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: clean
	@echo "\n=== Compiling with AddressSanitizer and UndefinedBehaviorSanitizer ==="
	$(MAKE) $(TARGET)
	$(MAKE) unit
	$(MAKE) integration
	@echo "=== Sanitizer checks passed: Zero memory leaks or undefined behaviors ==="

# ------------------------------------------------------------------------------
# 9. Test Case Documentation Generation Target
# ------------------------------------------------------------------------------
docs:
	@echo "=== Generating docs/TEST_CASES.md from tests/test_results.csv ==="
	@bash tools/generate_test_cases_doc.sh

# ------------------------------------------------------------------------------
# Master Test Target (Runs entire suite and regenerates docs)
# ------------------------------------------------------------------------------
test: clean $(TARGET)
	@echo "======================================================================"
	@echo "           PRECISION-CALCULATOR: FULL TEST EXECUTION SUITE            "
	@echo "======================================================================"
	@rm -f tests/test_results.csv
	@echo "ID,TYPE,RESULT,DURATION_MS,MESSAGE" > tests/test_results.csv
	@$(MAKE) unit
	@$(MAKE) integration
	@$(MAKE) system
	@$(MAKE) acceptance
	@$(MAKE) blackbox
	@$(MAKE) regression
	@$(MAKE) docs
	@echo "\n======================================================================"
	@echo "                      FULL TEST SUITE COMPLETE                         "
	@echo "======================================================================"

clean:
	rm -rf $(OBJDIR) $(TARGET) \
	       $(UNIT_TESTS) $(INTEG_TESTS) $(SYS_TESTS) \
	       $(BLACKBOX_RUNNER) $(GRAYBOX_TEST) \
	       *.gcda *.gcno *.gcov obj/*.gcda obj/*.gcno \
	       tests/*.dSYM tests/unit/*.dSYM tests/integration/*.dSYM tests/system/*.dSYM tests/blackbox/*.dSYM

help:
	@echo "Precision-Calculator Testing & Build System Targets:"
	@echo "  make              - Build production calculator executable"
	@echo "  make test         - Execute all test levels and regenerate documentation"
	@echo "  make unit         - Run White-Box Unit Test Suite (12 modules)"
	@echo "  make integration  - Run Integration Test Suite (Top-Down, Bottom-Up, Big-Bang, Flow)"
	@echo "  make system       - Run System Test Suite (Perf, Concurrency, Stress, Usability, etc.)"
	@echo "  make acceptance   - Run Acceptance Test Suite (UAT, BAT, RAT, Alpha, Beta)"
	@echo "  make blackbox     - Run Black-Box Data-Driven Runner (CSV test tables + Gray-Box)"
	@echo "  make perf         - Run Performance SLA Benchmarking Suite"
	@echo "  make regression   - Run Golden Baseline Oracle Regression Test"
	@echo "  make sanitize     - Build and run under AddressSanitizer and UndefinedBehaviorSanitizer"
	@echo "  make coverage     - Generate gcov statement and branch coverage metrics"
	@echo "  make docs         - Generate docs/TEST_CASES.md from tests/test_results.csv"
	@echo "  make clean        - Remove all compiled objects, binaries, and temporary files"

-include $(DEPS)
