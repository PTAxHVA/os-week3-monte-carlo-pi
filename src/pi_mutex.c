#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

long long g_inside = 0;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    long long n_points;
    long k;
    unsigned int seed;
    int error;
} WorkerArgs;

static int add_batch(long long batch)
{
    int rc = pthread_mutex_lock(&g_lock);
    if (rc != 0)
        return rc;

    g_inside += batch;
    return pthread_mutex_unlock(&g_lock);
}

static void *worker(void *arg)
{
    WorkerArgs *args = arg;
    unsigned int seed = args->seed;
    long long batch = 0;

    for (long long i = 0; i < args->n_points; i++) {
        double x = rand01(&seed);
        double y = rand01(&seed);
        if (x * x + y * y <= 1.0)
            batch++;

        if ((i + 1) % args->k == 0) {
            args->error = add_batch(batch);
            if (args->error != 0)
                return NULL;
            batch = 0;
        }
    }

    if (batch != 0)
        args->error = add_batch(batch);

    return NULL;
}

static void print_usage(void)
{
    fprintf(stderr,
            "Usage: ./pi_mutex <nPoints> <nThreads> <seed> [k]\n"
            "  nPoints >= nThreads >= 1, seed in [0, UINT_MAX], k >= 1\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4 && argc != 5) {
        print_usage();
        return 1;
    }

    errno = 0;
    char *end;
    long long n_points = strtoll(argv[1], &end, 10);
    if (errno != 0 || *end != '\0' || n_points <= 0) {
        print_usage();
        return 1;
    }

    errno = 0;
    long raw_threads = strtol(argv[2], &end, 10);
    if (errno != 0 || *end != '\0' || raw_threads < 1 ||
        raw_threads > INT_MAX || n_points < raw_threads) {
        print_usage();
        return 1;
    }
    int n_threads = (int)raw_threads;

    errno = 0;
    unsigned long long raw_seed = strtoull(argv[3], &end, 10);
    if (errno != 0 || *end != '\0' || raw_seed > UINT_MAX) {
        print_usage();
        return 1;
    }

    long k = 1;
    if (argc == 5) {
        errno = 0;
        k = strtol(argv[4], &end, 10);
        if (errno != 0 || *end != '\0' || k < 1) {
            print_usage();
            return 1;
        }
    }

    if ((size_t)n_threads > SIZE_MAX / sizeof(pthread_t) ||
        (size_t)n_threads > SIZE_MAX / sizeof(WorkerArgs)) {
        fprintf(stderr, "Error: too many threads\n");
        return 1;
    }

    pthread_t *threads = calloc((size_t)n_threads, sizeof(*threads));
    WorkerArgs *args = calloc((size_t)n_threads, sizeof(*args));
    if (threads == NULL || args == NULL) {
        fprintf(stderr, "Error: unable to allocate thread data\n");
        free(threads);
        free(args);
        return 1;
    }

    long long base = n_points / n_threads;
    long long remainder = n_points % n_threads;
    for (int i = 0; i < n_threads; i++) {
        args[i].n_points = base + (i < remainder);
        args[i].k = k;
        args[i].seed = (unsigned int)raw_seed + (unsigned int)i;
    }

    double t0 = now_sec();
    if (t0 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        free(threads);
        free(args);
        return 1;
    }

    int created = 0;
    for (; created < n_threads; created++) {
        int rc = pthread_create(&threads[created], NULL, worker, &args[created]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create for thread %d: %s\n",
                    created, strerror(rc));
            break;
        }
    }

    int failed = created != n_threads;
    for (int i = 0; i < created; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_join for thread %d: %s\n",
                    i, strerror(rc));
            failed = 1;
        } else if (args[i].error != 0) {
            fprintf(stderr, "Error: mutex operation in thread %d: %s\n",
                    i, strerror(args[i].error));
            failed = 1;
        }
    }

    long long inside = g_inside;
    double t1 = now_sec();
    if (t1 < 0.0) {
        fprintf(stderr, "Error: clock_gettime failed\n");
        failed = 1;
    }

    int destroy_rc = pthread_mutex_destroy(&g_lock);
    if (destroy_rc != 0) {
        fprintf(stderr, "Error: pthread_mutex_destroy: %s\n",
                strerror(destroy_rc));
        failed = 1;
    }

    free(threads);
    free(args);
    if (failed)
        return 1;

    print_result("mutex", n_threads, k, n_points, inside, t1 - t0);
    return 0;
}
