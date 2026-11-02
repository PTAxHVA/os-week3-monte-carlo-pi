#ifndef PI_COMMON_H
#define PI_COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * now_sec - wall-clock time in seconds (monotonic).
 *
 * Uses CLOCK_MONOTONIC so elapsed-time measurements are not affected
 * by NTP adjustments.  Returns -1.0 on error (caller should treat
 * that as fatal).
 */
static inline double now_sec(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return -1.0;
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/**
 * rand01 - pseudo-random double in [0, 1].
 *
 * Thread-safe because each caller owns its own seed.
 */
static inline double rand01(unsigned int *seed)
{
    return (double)rand_r(seed) / RAND_MAX;
}

/**
 * print_result - emit exactly one CSV data line to stdout.
 *
 * Format: approach,threads,k,points,inside,pi,time_sec
 * No header line — the benchmark script adds that.
 */
static inline void print_result(const char *approach, int threads, long k,
                                long long points, long long inside, double sec)
{
    printf("%s,%d,%ld,%lld,%lld,%.8f,%.6f\n",
           approach, threads, k, points, inside,
           4.0 * (double)inside / (double)points, sec);
}

#endif /* PI_COMMON_H */
