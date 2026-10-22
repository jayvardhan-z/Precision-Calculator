# Deployment Runbook & Infrastructure Guide

**Project Name:** Arbitrary-Precision Calculator & Scientific Computing Engine  
**Project Identifier:** Precision-Calculator  
**Version:** 2.0.0  
**Document Classification:** Production Operations & Release Runbook  

---

## 1. Build & Release Artifacts

The Precision-Calculator build system generates three deployment targets:
1. **Standalone CLI Binary (`calculator`):** Self-contained executable for interactive terminal REPL and batch execution.
2. **Static Archive Library (`libprecisioncalc.a`):** For statically linking into embedded systems or high-performance C/C++ applications.
3. **Shared Dynamic Library (`libprecisioncalc.so` / `libprecisioncalc.dylib`):** For dynamic loading via FFI bindings in Python, Go, Node.js, or Rust.

### 1.1 Generating Static and Shared Libraries
```bash
# Build standalone binary
make clean && make

# Build Static Archive
ar rcs libprecisioncalc.a obj/Worklet1/*.o obj/Worklet2/*.o obj/Worklet3/*.o

# Build Shared Dynamic Library (macOS)
cc -shared -o libprecisioncalc.dylib obj/Worklet1/*.o obj/Worklet2/*.o obj/Worklet3/*.o -lm

# Build Shared Dynamic Library (Linux)
cc -shared -fPIC -o libprecisioncalc.so obj/Worklet1/*.o obj/Worklet2/*.o obj/Worklet3/*.o -lm
```

---

## 2. Multi-Stage Container Deployment

For cloud environments, microservices, or containerized execution, use this multi-stage Docker build:

```dockerfile
# File: Dockerfile
# Stage 1: Build & Compile
FROM alpine:3.19 AS builder
RUN apk add --no-cache gcc make musl-dev

WORKDIR /app
COPY . .
RUN make clean && make CFLAGS="-O3 -D_POSIX_C_SOURCE=200809L -std=c11"

# Stage 2: Minimal Distroless / Scratch Runtime
FROM alpine:3.19 AS runtime
WORKDIR /usr/local/bin
COPY --from=builder /app/calculator .

# Run as non-root unprivileged user
RUN adduser -D -u 10001 calcuser
USER calcuser

ENTRYPOINT ["/usr/local/bin/calculator"]
CMD []
```

### Building & Running the Container:
```bash
# Build optimized image (< 10 MB total size)
docker build -t precision-calculator:v2.0.0 .

# Execute one-off calculation
docker run --rm precision-calculator:v2.0.0 "sin(pi / 4) + sqrt(2)"

# Launch interactive REPL inside container
docker run -it --rm precision-calculator:v2.0.0
```

---

## 3. Production Resource Constraints & Hardening

Because arbitrary-precision arithmetic can consume unbounded CPU or memory on hostile inputs (e.g., $9^{9^{9^9}}$), production deployments must enforce OS-level resource limits.

### 3.1 Kernel Resource Limits (`ulimit` / `systemd`)
When deploying the calculator as a backend daemon or invoking it via subprocesses, apply strict resource caps:

```bash
# Limit virtual address space to 512 MB to prevent system OOM
ulimit -v 524288

# Limit maximum CPU time to 5 seconds per calculation
ulimit -t 5

# Limit core dump file size to prevent disk filling
ulimit -c 0
```

### 3.2 Systemd Service Definition for Daemon Mode
```ini
[Unit]
Description=Precision Calculator Service Daemon
After=network.target

[Service]
Type=simple
User=calcuser
ExecStart=/usr/local/bin/calculator_daemon --port 8080
Restart=always
RestartSec=5s

# Hard Resource Limits
MemoryMax=512M
CPUQuota=100%
TasksMax=64
LimitNOFILE=1024

# Security Hardening
ProtectSystem=strict
ProtectHome=true
NoNewPrivileges=true

[Install]
WantedBy=multi-user.target
```

---

## 4. Incident Triage & Emergency Playbook

### Scenario A: High CPU Utilization / Runaway Calculation
* **Symptom:** A calculation worker process consumes 100% CPU for $> 5$ seconds.
* **Root Cause:** A user or script submitted an astronomically large exponentiation (e.g. `2^10000000`) or deep factorial recursion.
* **Immediate Mitigation:**
  ```bash
  # Identify PID consuming CPU
  top -b -n 1 | grep calculator
  
  # Send SIGTERM for graceful exit
  kill -15 <PID>
  
  # If unresponsive after 3 seconds, force terminate
  kill -9 <PID>
  ```
* **Long-Term Fix:** Ensure `bigint_factorial` operand limit ($N \le 10000$) and exponent bounds are verified in `eval_binary()` before invoking power routines.

### Scenario B: Out-of-Memory (OOM) Termination
* **Symptom:** Process terminates abruptly with exit code 137 (`SIGKILL` by kernel OOM killer).
* **Investigation:**
  ```bash
  dmesg -T | grep -i oom-killer
  ```
* **Mitigation:**
  * Adjust Docker container memory limit: `--memory="1g" --memory-swap="1g"`.
  * Verify that `eval_node()` and `ast_free()` are called symmetrically to avoid memory accumulation across REPL loops.

---

## 5. Release Checklist & Rollback Procedure

### 5.1 Pre-Release Verification Checklist
- [ ] Code compiles cleanly under GCC 11+ and Clang 15+ with `-Wall -Wextra -pedantic -std=c11`.
- [ ] AddressSanitizer (`-fsanitize=address,undefined`) passes all acceptance test suites.
- [ ] Valgrind confirms `0 leaks, 0 errors` across standard evaluation scenarios.
- [ ] Git working directory is clean and tagged with the semantic version (`git tag -a v2.0.0 -m "Release v2.0.0"`).
- [ ] Binary output verified: `./calculator "2+2"` yields `4`.

### 5.2 Emergency Rollback Procedure
If a regression or critical numerical discrepancy is detected in production:
1. **Revert Binary Symlink:**
   ```bash
   ln -sfn /usr/local/bin/calculator-v1.9.0 /usr/local/bin/calculator
   ```
2. **Revert Container Image:**
   ```bash
   docker service update --image precision-calculator:v1.9.0 calc_service
   ```
3. **Verify Restored Service:**
   ```bash
   /usr/local/bin/calculator "10/4" # Must print 2.5
   ```
4. **Post-Mortem:** File an issue detailing expression, expected result, actual result, and commit hash.
