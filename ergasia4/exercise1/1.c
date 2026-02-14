#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>

#define LOWER_BOUND -20
#define UPPER_BOUND 20

double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int* create_random_polynomial(int degree) {
    int *poly = (int *)malloc((size_t)(degree + 1) * sizeof(int));
    if(!poly) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i <= degree; ++i) {
        int r;
        do {
            r = (rand() % (UPPER_BOUND - LOWER_BOUND + 1)) + LOWER_BOUND;
        } while(r == 0);       // If 0, regenerate to avoid zero coefficients.
        poly[i] = r;
    }
    return poly;
}

int* multiply_sequential(const int *poly1, int deg1, const int *poly2, int deg2) {
    int result_len = deg1 + deg2 + 1;
    int *result = (int *)calloc((size_t)result_len, sizeof(int));
    if(!result) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i <= deg1; ++i) {
        for(int j = 0; j <= deg2; ++j) {
            result[i + j] += poly1[i] * poly2[j];
        }
    }
    return result;
}

int* multiply_simd(const int *poly1, int deg1, const int *poly2, int deg2) {
    int result_len = deg1 + deg2 + 1;
    int *result = (int *)calloc((size_t)result_len, sizeof(int));
    if(!result) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i <= deg1; ++i) {

        __m256i p1_vec = _mm256_set1_epi32(poly1[i]);

        int j = 0;
        for(; j <= deg2 - 7; j += 8) {
            __m256i p2_vec = _mm256_loadu_si256((__m256i*)&poly2[j]);

            __m256i result_vec = _mm256_loadu_si256((__m256i*)&result[i + j]);

            __m256i product = _mm256_mullo_epi32(p1_vec, p2_vec);

            result_vec = _mm256_add_epi32(result_vec, product);

            _mm256_storeu_si256((__m256i*)&result[i + j], result_vec);
        }

        for(; j <= deg2; ++j) {    // Handle remaining coefficients.
            result[i + j] += poly1[i] * poly2[j];
        }
    }
    return result;
}

int results_equal(const int *res1, const int *res2, int degree) {
    for (int i = 0; i <= degree; ++i) {
        if (res1[i] != res2[i]) {
            return 0;
        }
    }
    return 1;
}


int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <degree1> <degree2>\n", argv[0]);
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

    double simd_start = now_seconds();
    int *simd_result = multiply_simd(poly1, degree1, poly2, degree2);
    double simd_end = now_seconds();
    printf("SIMD multiplication took %.3f seconds\n", simd_end - simd_start);

    printf("Match baseline: %s\n", results_equal(baseline, simd_result, degree1 + degree2) ? "yes" : "no");

    free(poly1);
    free(poly2);
    free(baseline);
    free(simd_result);

    return EXIT_SUCCESS;
}