#ifndef TIMER_H
#define TIMER_H

#include <time.h>

// ============================================================================
// File: timer.h
// Purpose: High-resolution monotonic wall-clock timer for benchmarking expression
//          evaluation and arithmetic operations.
// Role:
//   Provides microsecond/nanosecond-level precision timing using POSIX
//   CLOCK_MONOTONIC to measure execution duration without being affected by
//   system clock adjustments or NTP synchronization jumps.
// ============================================================================

// Timer structure holding start timestamp and computed elapsed time in milliseconds
typedef struct {
    struct timespec start;      // POSIX monotonic start timestamp (seconds + nanoseconds)
    double          elapsed_ms; // Total elapsed duration in milliseconds after timer_stop
} Timer;

// Captures the current timestamp using CLOCK_MONOTONIC to begin timing
void timer_start(Timer *t);

// Computes the elapsed duration since timer_start in milliseconds and stores in t->elapsed_ms
void timer_stop(Timer *t);

// Retrieves the measured duration in milliseconds from the timer structure
double timer_elapsed_ms(const Timer *t);

#endif // TIMER_H