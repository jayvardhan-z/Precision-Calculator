#include <time.h>

typedef struct {
    struct timespec start;
    double elapsed_ms;
} Timer;

void timer_start(Timer *t);
void timer_stop(Timer *t);
double timer_elapsed_ms(const Timer *t);