#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOWER_BOUND -20
#define UPPER_BOUND 20

typedef struct {
    int start_i;
    int end_i;
    int degree1;
    int degree2;
    const int *poly1;
    const int *poly2;
    int *result_local;
} thread_data_t;

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

void *multiply_parallel_worker(void *arg)
{
    thread_data_t *td = (thread_data_t *)arg;
    for (int i = td->start_i; i <= td->end_i; ++i) {
        for (int j = 0; j <= td->degree2; ++j) {
            td->result_local[i + j] += td->poly1[i] * td->poly2[j];
        }
    }
    return NULL;
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

void print_polynomial(int *poly, int degree)
{
    printf("P(x) = ");
    int first_term_printed = 0; 

    for (int i = degree; i >= 0; i--) {
        int c = poly[i];
        if (c == 0) continue;

        int abs_c = (c < 0) ? -c : c;

        if (!first_term_printed) {
            if (c < 0) printf("-");
            first_term_printed = 1;
        }
        else {
            printf("%c ", (c < 0) ? '-' : '+');
        }

        if (i == 0) printf("%d ", abs_c);
        else if (i == 1) printf("%dx ", abs_c);
        else printf("%dx^%d ", abs_c, i);
    }
    printf("\n");
}

void run_parallel_case(int *poly1, int degree1, int *poly2, int degree2, int *baseline, int threads)
{
    int result_len = degree1 + degree2 + 1;
    double start = now_seconds();

    pthread_t thread_ids[threads];
    thread_data_t data[threads];
    int *locals[threads];

    for (int t = 0; t < threads; ++t) {
        locals[t] = (int *)calloc((size_t)result_len, sizeof(int));
        if (!locals[t]) {
            perror("calloc locals");
            exit(EXIT_FAILURE);
        }
    }

    int base = (degree1 + 1) / threads;
    int extra = (degree1 + 1) % threads;
    int offset = 0;

    for (int t = 0; t < threads; ++t) {
        int count = base + (t < extra ? 1 : 0);
        data[t].start_i = offset;
        data[t].end_i = offset + count - 1;
        data[t].degree1 = degree1;
        data[t].degree2 = degree2;
        data[t].poly1 = poly1;
        data[t].poly2 = poly2;
        data[t].result_local = locals[t];

        if (count > 0) {
            if (pthread_create(&thread_ids[t], NULL, multiply_parallel_worker, &data[t]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        else {
            data[t].start_i = 0;
            data[t].end_i = -1;
        }
        offset += count;
    }

    for (int t = 0; t < threads; ++t) {
        if (data[t].end_i >= data[t].start_i) {
            pthread_join(thread_ids[t], NULL);
        }
    }

    int *result_parallel = (int *)calloc((size_t)result_len, sizeof(int));
    if (!result_parallel) {
        perror("calloc result_parallel");
        exit(EXIT_FAILURE);
    }

    for (int t = 0; t < threads; ++t) {
        for (int k = 0; k < result_len; ++k) {
            result_parallel[k] += locals[t][k];
        }
        free(locals[t]);
    }

    double end = now_seconds();

    printf("Parallel multiplication with %d threads took %.3f seconds\n", threads, end - start);
    printf("Match baseline: %s\n", results_equal(baseline, result_parallel, degree1 + degree2) ? "yes" : "no");
    puts("---");

    free(result_parallel);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <degree1> <degree2> <threads...>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int degree1 = atoi(argv[1]);
    int degree2 = atoi(argv[2]);

    srand(time(NULL));

    double create_start = now_seconds();
    int *poly1 = create_random_polynomial(degree1);
    int *poly2 = create_random_polynomial(degree2);
    double create_end = now_seconds();
    printf("Generated polynomials in %.3f seconds\n", create_end - create_start);

    double seq_start = now_seconds();
    int *baseline = multiply_sequential(poly1, degree1, poly2, degree2);
    double seq_end = now_seconds();
    printf("Sequential multiplication took %.3f seconds\n", seq_end - seq_start);

    for (int arg = 3; arg < argc; ++arg) {
        int threads = atoi(argv[arg]);
        if (threads <= 0) {
            fprintf(stderr, "Thread count must be positive (got %d).\n", threads);
            continue;
        }
        run_parallel_case(poly1, degree1, poly2, degree2, baseline, threads);
    }

    free(poly1);
    free(poly2);
    free(baseline);

    return EXIT_SUCCESS;
}