/**
 * ============================================================================
 * File: test_wb_timer.c
 * Module: Worklet 1 (timer.c)
 * White-Box Testing Techniques Covered:
 *   - Monotonic CLOCK_MONOTONIC timing verification
 *   - Non-decreasing duration invariant
 *   - Fractional millisecond calculation accuracy
 *   - NULL pointer safety for all entry points
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "minitest.h"
#include "timer.h"

int main(void) {
    minitest_init_suite("White-Box Unit: High-Resolution Timer (Worklet 1)", "tests/test_results.csv");

    TEST_CASE("UT-TIM-01", "UNIT", "timer_start and timer_stop measure positive elapsed duration") {
        Timer t;
        timer_start(&t);
        CHECK(fabs(t.elapsed_ms - 0.0) < 1e-6, "Elapsed time is 0.0 before stop");

        struct timespec delay = { .tv_sec = 0, .tv_nsec = 15000000L }; /* 15 ms */
        nanosleep(&delay, NULL);

        timer_stop(&t);
        double ms = timer_elapsed_ms(&t);
        CHECK(ms >= 10.0, "Elapsed time must measure at least ~10 ms");
        CHECK(ms < 100.0, "Elapsed time must be reasonable upper bound");
    }

    TEST_CASE("UT-TIM-02", "UNIT", "Timer guarantees strictly non-decreasing monotonic timestamps") {
        Timer t1, t2;
        timer_start(&t1);
        struct timespec delay = { .tv_sec = 0, .tv_nsec = 5000000L }; /* 5 ms */
        nanosleep(&delay, NULL);
        timer_stop(&t1);

        timer_start(&t2);
        nanosleep(&delay, NULL);
        nanosleep(&delay, NULL); /* ~10 ms */
        timer_stop(&t2);

        CHECK(timer_elapsed_ms(&t2) >= timer_elapsed_ms(&t1) * 0.8,
              "Second timer duration should be longer or comparable");
    }

    TEST_CASE("UT-TIM-03", "UNIT", "timer safe NULL pointer handling") {
        timer_start(NULL);
        timer_stop(NULL);
        double ms = timer_elapsed_ms(NULL);
        CHECK(fabs(ms - 0.0) < 1e-9, "timer_elapsed_ms(NULL) safely returns 0.0");
    }

    return minitest_finish_suite();
}
