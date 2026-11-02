/* High-precision Monte Carlo pi: 64-bit xoshiro256+ per thread (seeded by splitmix64),
 * 53-bit uniform doubles, 64-bit counters. Usage: ./pi_big <nPoints> <nThreads> <seed> */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct { uint64_t s[4]; long long n, inside; } Task;

static uint64_t splitmix64(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
static inline uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
static inline double next01(uint64_t *s) {            /* xoshiro256+ -> [0,1) with 53 bits */
    uint64_t r = s[0] + s[3], t = s[1] << 17;
    s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3]; s[2] ^= t; s[3] = rotl(s[3], 45);
    return (double)(r >> 11) * 0x1.0p-53;
}
static void *worker(void *arg) {
    Task *t = arg; uint64_t s[4] = {t->s[0], t->s[1], t->s[2], t->s[3]}; long long in = 0;
    for (long long i = 0; i < t->n; i++) { double x = next01(s), y = next01(s); in += (x * x + y * y <= 1.0); }
    t->inside = in; return NULL;
}
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Usage: %s <nPoints> <nThreads> <seed>\n", argv[0]); return 1; }
    char *e1, *e2, *e3; errno = 0;
    long long n = strtoll(argv[1], &e1, 10); long T = strtol(argv[2], &e2, 10);
    unsigned long long seed = strtoull(argv[3], &e3, 10);
    if (errno || *e1 || *e2 || *e3 || n < 1 || T < 1 || T > 1024 || n < T) {
        fprintf(stderr, "Usage: %s <nPoints> <nThreads> <seed>\n", argv[0]); return 1; }
    Task *tk = calloc(T, sizeof *tk); pthread_t *th = calloc(T, sizeof *th);
    if (!tk || !th) { perror("calloc"); return 1; }
    uint64_t sm = seed;                                 /* one splitmix stream -> distinct thread states */
    for (long i = 0; i < T; i++) {
        for (int j = 0; j < 4; j++) tk[i].s[j] = splitmix64(&sm);
        tk[i].n = n / T + (i < n % T);
    }
    double t0 = now();
    for (long i = 0; i < T; i++) if (pthread_create(&th[i], NULL, worker, &tk[i])) { perror("pthread_create"); return 1; }
    long long in = 0;
    for (long i = 0; i < T; i++) { pthread_join(th[i], NULL); in += tk[i].inside; }
    double dt = now() - t0, p = 4.0 * (double)in / (double)n;
    double se = 4.0 * sqrt((p / 4) * (1 - p / 4) / (double)n);
    printf("big,%ld,0,%lld,%lld,%.10f,%.3f,%.2e\n", T, n, in, p, dt, se);
    free(tk); free(th); return 0;
}
