#include "common.h"
#include <pthread.h>

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
        double x = 2.0 * rand01(&seed) - 1.0;
        double y = 2.0 * rand01(&seed) - 1.0;
        if (x * x + y * y <= 1.0) {
            local_inside++;
        }
    }

    arg->inside = local_inside;
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <nPoints> <nThreads> [seed]\n", argv[0]);
        return EXIT_FAILURE;
    }

    long long nPoints = atoll(argv[1]);
    int nThreads      = atoi(argv[2]);
    unsigned int base_seed = (argc >= 4) ? (unsigned int)atoi(argv[3]) : 42U;

    if (nPoints <= 0 || nThreads <= 0) {
        fprintf(stderr, "Invalid arguments: nPoints and nThreads must be > 0\n");
        return EXIT_FAILURE;
    }

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
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    for (int t = 0; t < nThreads; t++) {
        if (pthread_create(&threads[t], NULL, worker, &args[t]) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    long long total_inside = 0;
    for (int t = 0; t < nThreads; t++) {
        pthread_join(threads[t], NULL);
        total_inside += args[t].inside;
    }

    double t1 = now_sec();
    if (t1 < 0.0) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    print_result("local", nThreads, 0, nPoints, total_inside, t1 - t0);

    free(threads);
    free(args);
    return EXIT_SUCCESS;
}