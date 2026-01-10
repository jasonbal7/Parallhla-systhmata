#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define LOWER_BOUND -20
#define UPPER_BOUND 20

int *create_random_polynomial(int degree)
{
    int *poly = (int *)malloc((size_t)(degree + 1) * sizeof(int));
    if (!poly) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= degree; ++i) {
        poly[i] = (rand() % (UPPER_BOUND - LOWER_BOUND + 1)) + LOWER_BOUND;
    }
    return poly;
}

int *multiply_sequential(const int *poly1, int deg1, const int *poly2, int deg2)
{
    int result_len = deg1 + deg2 + 1;
    int *result = (int *)calloc((size_t)result_len, sizeof(int));
    if (!result) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= deg1; ++i) {
        for (int j = 0; j <= deg2; ++j) {
            result[i + j] += poly1[i] * poly2[j];
        }
    }
    return result;
}

int results_equal(const int *res1, const int *res2, int degree)
{
    for (int i = 0; i <= degree; ++i) {
        if (res1[i] != res2[i]) {
            return 0;
        }
    }
    return 1;
}

double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void run_parallel(int *p1, int d1, int *p2, int d2, int threads, int *baseline)
{
    int result_len = d1 + d2 + 1;
    double start = now_seconds();

    //locals is a contiguous 2D buffer: locals[tid][k] at locals + tid*result_len + k.
    int *locals = (int *)calloc((size_t)threads * (size_t)result_len, sizeof(int));
    if (!locals) {
        perror("calloc locals");
        exit(EXIT_FAILURE);
    }

    #pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        int thread_count = omp_get_num_threads();
        int *local = locals + (size_t)tid * (size_t)result_len;

        for (int i = tid; i <= d1; i += thread_count) {
            for (int j = 0; j <= d2; ++j) {
                local[i + j] += p1[i] * p2[j];
            }
        }
    }

    int *result_parallel = (int *)calloc((size_t)result_len, sizeof(int));
    if (!result_parallel) {
        perror("calloc result_parallel");
        free(locals);
        exit(EXIT_FAILURE);
    }

    for (int t = 0; t < threads; ++t) {
        int *local = locals + (size_t)t * (size_t)result_len;
        for (int k = 0; k < result_len; ++k) {
            result_parallel[k] += local[k];
        }
    }

    free(locals);
    double end = now_seconds();
    printf("Parallel multiplication with %d threads took %.3f seconds\n", threads, end - start);
    printf("Match baseline: %s\n", results_equal(baseline, result_parallel, d1 + d2) ? "yes" : "no");
    free(result_parallel);
}

int main(int argc, char *argv[]) 
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <degree1> <degree2> <threads...>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int d1 = atoi(argv[1]);
    int d2 = atoi(argv[2]);

    srand(time(NULL));

    double create_start = now_seconds();
    int *poly1 = create_random_polynomial(d1);
    int *poly2 = create_random_polynomial(d2);
    double create_end = now_seconds();
    printf("Generated polynomials in %.3f seconds\n", create_end - create_start);
    
    double seq_start = now_seconds();
    int *baseline = multiply_sequential(poly1, d1, poly2, d2);
    double seq_end = now_seconds();
    printf("Sequential multiplication took %.3f seconds\n", seq_end - seq_start);

    for (int i = 3; i < argc; ++i) {
        int threads = atoi(argv[i]);
        run_parallel(poly1, d1, poly2, d2, threads, baseline);
    }

    free(poly1);
    free(poly2);
    free(baseline);

    return 0;
}