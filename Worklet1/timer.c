#define _POSIX_C_SOURCE 200809L
#include "timer.h"

// remember the current time
void timer_start(Timer *t) {
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

// work out how many milliseconds passed since timer_start
void timer_stop(Timer *t) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double sec = (double)(now.tv_sec - t->start.tv_sec);
    double nsec = (double)(now.tv_nsec - t->start.tv_nsec);
    t->elapsed_ms = sec * 1000.0 + nsec / 1000000.0;
}

double timer_elapsed_ms(const Timer *t) {
    return t->elapsed_ms;
}