CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c11 -O2
CPPFLAGS = -IWorklet1 -IWorklet2 -IWorklet3 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
LDLIBS = -lm

TARGET = calculator
OBJDIR = obj

# Core calculator sources
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

# Core library sources (excluding main.c) for building tests
LIB_SRCS = $(filter-out main.c, $(SRCS))

OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all run clean debug test test-unit test-extreme test-stress asan help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

# Run target compiles and launches the calculator REPL
run: $(TARGET)
	./$(TARGET)

# Debug build with sanitizers and debug symbols
debug: CFLAGS = -Wall -Wextra -pedantic -std=c11 -g3 -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

# ── Worklet 3 legacy test runners ───────────────────────────────────────────
TEST_PART5 = test_part5
TEST_PART6 = test_part6
TEST_PART8 = test_part8

LIB_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(LIB_SRCS))

$(TEST_PART5): $(OBJDIR)/Worklet3/test_part5.o $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TEST_PART6): $(OBJDIR)/Worklet3/test_part6.o $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TEST_PART8): $(OBJDIR)/Worklet3/test_part8.o $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: $(TEST_PART5) $(TEST_PART6)
	@echo "=== Running Worklet 3 Part 5 Tests ==="
	./$(TEST_PART5)
	@echo "=== Running Worklet 3 Part 6 Tests ==="
	./$(TEST_PART6)

# ── New extreme test suite ────────────────────────────────────────────────────
# Individual C unit test binaries (built from tests/ against all library sources)
TEST_UNIT_SRCS = tests/test_bigint.c tests/test_lexer_parser.c tests/test_eval.c
TEST_UNIT_BINS = $(patsubst tests/%.c, tests/%, $(TEST_UNIT_SRCS))

# Build a single unit test binary: compile test source + all library sources
tests/%: tests/%.c $(LIB_SRCS)
	@mkdir -p tests
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# test-unit: build and execute all three C unit test binaries
test-unit: $(TEST_UNIT_BINS)
	@echo ""
	@echo "=== Running Unit Tests: BigInteger ==="
	./tests/test_bigint
	@echo ""
	@echo "=== Running Unit Tests: Lexer + Parser ==="
	./tests/test_lexer_parser
	@echo ""
	@echo "=== Running Unit Tests: Evaluator ==="
	./tests/test_eval

# test-extreme: full extreme test suite (unit + integration + ASAN + bench + diff)
test-extreme: all $(TEST_UNIT_BINS)
	chmod +x tests/run_tests.sh
	bash tests/run_tests.sh

# test-stress: automated extreme stress test suite (10k ops, 100k digits, 5k nesting)
test-stress: all
	chmod +x tests/extreme_stress_test.py
	python3 tests/extreme_stress_test.py

# asan: rebuild the entire project + all unit tests under AddressSanitizer + UBSan
asan: CFLAGS = -Wall -Wextra -pedantic -std=c11 -g3 -O0 \
              -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean $(TARGET) $(TEST_UNIT_BINS)
	@echo ""
	@echo "=== ASAN Build complete — now run: make test-unit ==="

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_PART5) $(TEST_PART6) $(TEST_PART8) \
	        $(TEST_UNIT_BINS) tests/asan_* tests/*.dSYM *.dSYM

help:
	@echo "Available make targets:"
	@echo "  make              - Build the calculator executable ($(TARGET))"
	@echo "  make run          - Build and run the calculator interactive REPL"
	@echo "  make test         - Build and run Worklet 3 legacy unit tests"
	@echo "  make test-unit    - Build and run all C unit tests (BigInt, Lexer, Eval)"
	@echo "  make test-extreme - Full extreme test suite: unit+integration+ASAN+bench"
	@echo "  make test-stress  - Automated stress suite (10k ops, 100k digits, 5k nesting)"
	@echo "  make asan         - Rebuild everything under AddressSanitizer+UBSan"
	@echo "  make debug        - Build with ASAN, UBSan, and debug symbols (-g3)"
	@echo "  make clean        - Remove all build artifacts and test binaries"
	@echo "  make help         - Display this information"

-include $(DEPS)

