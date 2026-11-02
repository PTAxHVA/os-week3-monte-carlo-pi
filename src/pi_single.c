#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: ./pi_single <nPoints> <seed>\n");
        return 1;
    }

    /* ---- parse nPoints ---- */
    errno = 0;
    char *end;
    long long nPoints = strtoll(argv[1], &end, 10);
    if (errno != 0 || *end != '\0' || nPoints <= 0) {
        fprintf(stderr, "Usage: ./pi_single <nPoints> <seed>\n");
        return 1;
    }

    /* ---- parse seed ---- */
    errno = 0;
    unsigned long long raw_seed = strtoull(argv[2], &end, 10);
    if (errno != 0 || *end != '\0' || raw_seed > UINT_MAX) {
        fprintf(stderr, "Usage: ./pi_single <nPoints> <seed>\n");
        return 1;
    }

    unsigned int state = (unsigned int)raw_seed;
    long long inside = 0;

    /* ---- timed section ---- */
    double t0 = now_sec();
    if (t0 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        return 1;
    }

    for (long long i = 0; i < nPoints; i++) {
        double x = rand01(&state);
        double y = rand01(&state);
        if (x * x + y * y <= 1.0)
            inside++;
    }

    double t1 = now_sec();
    if (t1 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        return 1;
    }

    print_result("single", 1, 0, nPoints, inside, t1 - t0);
    return 0;
}
