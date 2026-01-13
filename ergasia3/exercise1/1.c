#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <mpi.h>
#include <string.h>

#define LOWER_BOUND -20
#define UPPER_BOUND 20

int *create_random_polynomial_seeded(int degree, unsigned int *seed)
{
    int *poly = (int *)malloc((size_t)(degree + 1) * sizeof(int));
    if (!poly) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= degree; ++i) {
        int r = (int)(rand_r(seed) % (UPPER_BOUND - LOWER_BOUND + 1));
        poly[i] = r + LOWER_BOUND;
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
        int a = poly1[i];
        int *res = result + i;
        for (int j = 0; j <= deg2; ++j) {
            res[j] += a * poly2[j];
        }
    }
    return result;
}

int *multiply_parallel_mpi_slice(const int *poly2, int d2, int *local_poly1, int local_start, int local_len)
{
    int result_len = local_start + local_len + d2 + 1; 
    int *local_result = (int *)calloc(result_len, sizeof(int));
    if (!local_result) { 
        perror("calloc");
         exit(EXIT_FAILURE); 
    }

    for (int i = 0; i < local_len; ++i) {
        int a = local_poly1[i];
        int *res = local_result + i;
        for (int j = 0; j <= d2; ++j) {
            res[j] += a * poly2[j];
        }
    }

    return local_result;
}

double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) 
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) fprintf(stderr, "Usage: %s <degree> [seed]\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    unsigned int seed = 12345u;
    if (argc >= 3) seed = (unsigned int)strtoul(argv[2], NULL, 10);

    int *poly1 = NULL;
    int *poly2 = NULL;

    if (rank == 0) {
        unsigned int seed1 = seed;
        unsigned int seed2 = seed ^ 0x9e3779b9u;
        poly1 = create_random_polynomial_seeded(n, &seed1);
        poly2 = create_random_polynomial_seeded(n, &seed2);
    } 
    else {
        poly2 = malloc((size_t)(n + 1) * sizeof(int));
        if (!poly2) { perror("malloc"); MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); }
    }

    //sequential multiplication 
    if (rank == 0) {
        double t0 = now_seconds();
        int *tmp = multiply_sequential(poly1, n, poly2, n);
        double t1 = now_seconds();
        printf("Sequential time: %.6f seconds\n", t1 - t0);
        free(tmp);
    }

    double t_total_start = MPI_Wtime();
    double t_send_start = MPI_Wtime();

    //broadcast poly2 to all processes
    MPI_Bcast(poly2, n + 1, MPI_INT, 0, MPI_COMM_WORLD);

    //determine local slice for each process
    int chunk_size = (n + 1 + size - 1) / size;         //ceiling division
    int local_start = rank * chunk_size;
    int local_end = (local_start + chunk_size < n + 1) ? local_start + chunk_size : n + 1;
    int local_len = local_end - local_start;

    int *local_poly1 = malloc((size_t)local_len * sizeof(int));
    if (!local_poly1) { perror("malloc"); MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); }

    if (rank == 0) {
        //copy own slice
        memcpy(local_poly1, poly1 + local_start, (size_t)local_len * sizeof(int));
        //end slices to other processes
        for (int p = 1; p < size; ++p) {
            int p_start = p * chunk_size;
            int p_end = (p_start + chunk_size < n + 1) ? p_start + chunk_size : n + 1;
            int p_len = p_end - p_start;
            MPI_Send(poly1 + p_start, p_len, MPI_INT, p, 0, MPI_COMM_WORLD);
        }
    } 
    else {
        MPI_Recv(local_poly1, local_len, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    double t_send_end = MPI_Wtime();

    //parallel local multiplication
    double t_comp_start = MPI_Wtime();
    int local_res_len = local_len + n + 1;
    int *local_result = calloc((size_t)local_res_len, sizeof(int));
    if (!local_result) { perror("calloc"); MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); }

    for (int i = 0; i < local_len; ++i) {
        int a = local_poly1[i];
        int *res = local_result + i;
        for (int j = 0; j <= n; ++j)
            res[j] += a * poly2[j];
    }
    double t_comp_end = MPI_Wtime();

    //reduce only relevant slice of results
    double t_recv_start = MPI_Wtime();
    int *global_result = NULL;
    if (rank == 0) global_result = calloc((size_t)(2 * n + 1), sizeof(int));
    if (rank == 0) {
        MPI_Reduce(MPI_IN_PLACE, global_result + local_start, local_res_len, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    } 
    else {
        MPI_Reduce(local_result, global_result + local_start, local_res_len, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    double t_recv_end = MPI_Wtime();
    double t_total_end = MPI_Wtime();

    if (rank == 0) {
        printf("Time to send slices: %.6f s\n", t_send_end - t_send_start);
        printf("Parallel computation: %.6f s\n", t_comp_end - t_comp_start);
        printf("Time to gather results: %.6f s\n", t_recv_end - t_recv_start);
        printf("Total parallel: %.6f s\n", t_total_end - t_total_start);

        free(global_result);
        free(poly1);
        free(poly2);
    }

    free(local_poly1);
    free(local_result);
    if (rank != 0) free(poly2);

    MPI_Finalize();
    return 0;
}