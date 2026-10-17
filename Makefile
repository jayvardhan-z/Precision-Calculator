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

.PHONY: all run clean debug test help

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

# Test runners for Worklet3 parts
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

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_PART5) $(TEST_PART6) $(TEST_PART8)

help:
	@echo "Available make targets:"
	@echo "  make          - Build the calculator executable ($(TARGET))"
	@echo "  make run      - Build and run the calculator interactive REPL"
	@echo "  make test     - Build and run unit tests"
	@echo "  make debug    - Build with AddressSanitizer and debug symbols"
	@echo "  make clean    - Remove build artifacts and binaries"
	@echo "  make help     - Display this information"

-include $(DEPS)

