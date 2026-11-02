#include "common.h"
#include <pthread.h>

typedef struct {
    int tid;
    long long points;
    unsigned int seed;
    long long inside; /* Ghi 1 lần cuối vòng lặp để tránh False Sharing */
} thread_arg_t;

static void *worker(void *arg_ptr)
{
    thread_arg_t *arg = (thread_arg_t *)arg_ptr;

    /* Giữ biến trên thanh ghi/stack cục bộ của luồng trong suốt vòng lặp[cite: 4] */
    unsigned int seed = arg->seed;
    long long n = arg->points;
    long long local_inside = 0;

    for (long long i = 0; i < n; i++) {
        /* Lưu ý: rand01(&seed) trong [0, 1] hay quy đổi ra [-1, 1] (2*rand01 - 1)
         * đều cho cùng kết quả hình học. Bạn nhớ thống nhất với người #3 (pi_mutex)
         * xem có nhân 2.0 - 1.0 hay giữ nguyên [0, 1] để khớp tuyệt đối số inside! */
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
    /* Cho phép truyền <nPoints> <nThreads> [seed] (mặc định seed = 42 nếu chỉ truyền 2 tham số) */
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

    /* Cấp phát và chia việc ngoài vùng đo thời gian[cite: 6] */
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
        /* Chia phần dư cho rem_pts luồng đầu tiên (hoặc thống nhất với người #3) */
        args[t].points = base_pts + (t < rem_pts ? 1 : 0);
        args[t].seed   = base_seed + (unsigned int)t;
        args[t].inside = 0;
    }

    /* Bắt đầu bấm giờ (Map + Reduce) */
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

    /* In kết quả theo đúng chuẩn CSV trong common.h (tham số k = 0 cho local) */
    print_result("local", nThreads, 0, nPoints, total_inside, t1 - t0);

    free(threads);
    free(args);
    return EXIT_SUCCESS;
}