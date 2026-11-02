#include "common.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef struct {
    int tid;
    long long points;
    unsigned int seed;
    long long inside;
} thread_arg_t;

static void *worker(void *arg_ptr)
{
    thread_arg_t *arg = (thread_arg_t *)arg_ptr;

    unsigned int seed = arg->seed;
    long long n = arg->points;
    long long local_inside = 0;

    for (long long i = 0; i < n; i++) {
        double x = rand01(&seed);
        double y = rand01(&seed);
        if (x * x + y * y <= 1.0) {
            local_inside++;
        }
    }

    arg->inside = local_inside;
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <nPoints> <nThreads> <seed>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr;
    errno = 0;
    long long nPoints = strtoll(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || nPoints <= 0) {
        fprintf(stderr, "Error: Invalid nPoints argument.\n");
        return EXIT_FAILURE;
    }

    errno = 0;
    long long nThreads_ll = strtol(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || nThreads_ll <= 0 || nThreads_ll > INT_MAX) {
        fprintf(stderr, "Error: Invalid nThreads argument.\n");
        return EXIT_FAILURE;
    }
    int nThreads = (int)nThreads_ll;

    errno = 0;
    unsigned long seed_ul = strtoul(argv[3], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || seed_ul > UINT_MAX) {
        fprintf(stderr, "Error: Invalid seed argument.\n");
        return EXIT_FAILURE;
    }
    unsigned int base_seed = (unsigned int)seed_ul;

    pthread_t *threads = malloc((size_t)nThreads * sizeof(*threads));
    thread_arg_t *args = malloc((size_t)nThreads * sizeof(*args));
    if (!threads || !args) {
        perror("malloc");
        free(threads);
        free(args);
        return EXIT_FAILURE;
    }

    long long base_pts = nPoints / nThreads;
    long long rem_pts  = nPoints % nThreads;

    for (int t = 0; t < nThreads; t++) {
        args[t].tid    = t;
        args[t].points = base_pts + (t < rem_pts ? 1 : 0);
        args[t].seed   = base_seed + (unsigned int)t;
        args[t].inside = 0;
    }

    double t0 = now_sec();
    if (t0 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        free(threads);
        free(args);
        return EXIT_FAILURE;
    }

    for (int t = 0; t < nThreads; t++) {
        int rc = pthread_create(&threads[t], NULL, worker, &args[t]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed for thread %d: %s\n", t, strerror(rc));
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
    }

    long long total_inside = 0;
    for (int t = 0; t < nThreads; t++) {
        int rc = pthread_join(threads[t], NULL);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_join failed for thread %d: %s\n", t, strerror(rc));
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
        total_inside += args[t].inside;
    }

    double t1 = now_sec();
    if (t1 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        free(threads);
        free(args);
        return EXIT_FAILURE;
    }

    print_result("local", nThreads, 0, nPoints, total_inside, t1 - t0);

    free(threads);
    free(args);
    return EXIT_SUCCESS;
}