#define _POSIX_C_SOURCE 200809L
#include "timer.h"

// ============================================================================
// File: timer.c
// Purpose: Implementation of high-precision monotonic timing routines.
// Role:
//   Measures wall-clock time spent during expression evaluation in the REPL
//   using the POSIX monotonic clock, which guarantees strictly non-decreasing
//   timestamps immune to system time changes.
// ============================================================================

// Records the current monotonic clock timestamp into the timer's start field
void timer_start(Timer *t) {
    if (!t) return;
    // Capture seconds and nanoseconds from CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &t->start);
    // Initialize elapsed_ms to zero until timer_stop is called
    t->elapsed_ms = 0.0;
}

// Computes the elapsed wall-clock time since timer_start in fractional milliseconds
void timer_stop(Timer *t) {
    if (!t) return;

    struct timespec now;
    // Query current monotonic timestamp
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Compute differential seconds between stop and start
    double sec = (double)(now.tv_sec - t->start.tv_sec);
    // Compute differential nanoseconds between stop and start
    double nsec = (double)(now.tv_nsec - t->start.tv_nsec);

    // Combine seconds (multiplied by 1000) and nanoseconds (divided by 1,000,000) into milliseconds
    t->elapsed_ms = sec * 1000.0 + (nsec / 1000000.0);
}

// Returns the measured elapsed time in milliseconds
double timer_elapsed_ms(const Timer *t) {
    if (!t) return 0.0;
    // Return precomputed millisecond duration
    return t->elapsed_ms;
}