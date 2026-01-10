#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

typedef struct {
    int *values;
    int *col_index;
    int *row_ptr;
    int nnz;
} CSRMatrix;

double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int **create_parse_array(int m, int n, double sparsity) 
{
    int **array = (int **)malloc(m *sizeof(int *));
    for (int i = 0; i < m; i++) {
        array[i] = (int *)malloc(n * sizeof(int));
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {

            double r = (double) rand() / RAND_MAX;

            if (r < sparsity) { 
                array[i][j] = 0;
            }
            else {
                array[i][j] = rand() % 100; 
            }
        }
    }
    return array;
}

int *create_vector(int n) 
{
    int *vector = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        vector[i] = rand() % 100;
    }
    return vector; 
}

CSRMatrix convert_to_csr(int **array, int m, int n) 
{
    CSRMatrix csr;
    int nnz = 0;

    //step 1 count non zero
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (array[i][j] != 0) {
                nnz++;
            }  
        }
    }
    csr.nnz = nnz;

    //step 2 create csr arrays
    csr.values = (int *)malloc(nnz * sizeof(int));
    csr.col_index = (int *)malloc(nnz * sizeof(int));
    csr.row_ptr = (int *)malloc((m + 1) * sizeof(int));

    //step 3 fill csr arrays
    int k = 0;
    csr.row_ptr[0] = 0;

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (array[i][j] != 0) {
                csr.values[k] = array[i][j];
                csr.col_index[k] = j;
                k++;
            }
        }
        csr.row_ptr[i + 1] = k;
    }

    return csr;
}

CSRMatrix convert_to_csr_omp(int **array, int m, int n) 
{
    CSRMatrix csr;
    int nnz = 0;

    //step 1 count non zero per row
    int *row_nnz = (int *)calloc(m, sizeof(int));
    #pragma omp parallel for 
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (array[i][j] != 0) {
                row_nnz[i]++;
            }
        }
    }

    //step 2 compute row_ptr serial prefix sum
    csr.row_ptr = (int *)malloc((m + 1) * sizeof(int));
    csr.row_ptr[0] = 0;
    for (int i = 0; i < m; i++) {
        csr.row_ptr[i + 1] = csr.row_ptr[i] + row_nnz[i];
    }
    csr.nnz = csr.row_ptr[m];

    //step 3 create csr arrays
    csr.values = (int *)malloc(csr.nnz * sizeof(int));
    csr.col_index = (int *)malloc(csr.nnz * sizeof(int));

    //step 4 fill csr arrays in parallel   
    #pragma omp parallel for 
    for (int i = 0; i < m; i++) {
        int idx = csr.row_ptr[i];
        for (int j = 0; j < n; j++) {
            if (array[i][j] != 0) { 
                csr.values[idx] = array[i][j];
                csr.col_index[idx] = j;
                idx++;
            }
        }
    }

    free(row_nnz);
    return csr;
}

int *dense_multiply(int **array, int *vector, int m)
{
    int *result = (int *)calloc(m, sizeof(int));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            result[i] += array[i][j] * vector[j];
        }
    }
    return result;
}

int *dense_multiply_omp(int **array, int *vector, int m)
{
    int *result = (int *)calloc(m, sizeof(int));

    #pragma omp parallel for
    for (int i = 0; i < m; i++) {
        int sum = 0;
        for (int j = 0; j < m; j++) {
            sum += array[i][j] * vector[j];
        }
        result[i] = sum;
    }

    return result;
}

int *csr_multiply(CSRMatrix csr, int *vector, int m)
{
    int *result = (int *)calloc(m, sizeof(int));
    for (int i = 0; i < m; i++) {
        for (int j = csr.row_ptr[i]; j < csr.row_ptr[i + 1]; j++) {
            result[i] += csr.values[j] * vector[csr.col_index[j]];
        }
    }
    return result;
}

int *csr_multiply_omp(CSRMatrix csr, int *vector, int m)
{
    int *result = (int *)calloc(m, sizeof(int));

    #pragma omp parallel for
    for (int i = 0; i < m; i++) {
        int sum = 0;
        for (int j = csr.row_ptr[i]; j < csr.row_ptr[i + 1]; j++) {
            sum += csr.values[j] * vector[csr.col_index[j]];
        }
        result[i] = sum;
    }

    return result;
}

int check_results(int *res1, int *res2, int m) 
{
    for (int i = 0; i < m; i++) {
        if (res1[i] != res2[i]) {
            return 0;
        }
    }
    return 1;
}

int csr_equal(CSRMatrix *csr1, CSRMatrix *csr2, int m) 
{
    if (csr1->nnz != csr2->nnz) return 0;

    for (int i = 0; i <= m; i++) {
        if (csr1->row_ptr[i] != csr2->row_ptr[i]) return 0;
    }
    for (int i = 0; i < csr1->nnz; i++) {
        if (csr1->values[i] != csr2->values[i]) return 0;
        if (csr1->col_index[i] != csr2->col_index[i]) return 0;
    }
    return 1;
}

void print_csr(CSRMatrix csr, int m) 
{
    printf("Values: ");
    for (int i = 0; i < csr.nnz; i++) {
        printf("%d ", csr.values[i]);
    }
    printf("\n");

    printf("Column Indices: ");
    for (int i = 0; i < csr.nnz; i++) {
        printf("%d ", csr.col_index[i]);
    }
    printf("\n");

    printf("Row Pointers: ");
    for (int i = 0; i <= m; i++) {
        printf("%d ", csr.row_ptr[i]);
    }
    printf("\n");
}

void print_array(int **array, int m, int n)
{
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", array[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <rows/cols> <sparsity> <iterations> <threads>\n", argv[0]);
        return 1;
    }
    int m = atoi(argv[1]);
    double sparsity = atof(argv[2]);
    int iterations = atoi(argv[3]);
    int threads = atoi(argv[4]);

    srand(time(NULL));
    omp_set_num_threads(threads);

    int **array = create_parse_array(m, m, sparsity);
    int *vector = create_vector(m);

    double start = now_seconds();
    CSRMatrix csr = convert_to_csr(array, m, m);
    double end = now_seconds();
    printf("Serial conversion to CSR in %.6f seconds\n", end - start);

    start = now_seconds();
    CSRMatrix omp_csr = convert_to_csr_omp(array, m, m);
    end = now_seconds();
    printf("Parallel conversion to CSR in %.6f seconds\n", end - start);
    printf("CSR structures match: %s\n", csr_equal(&csr, &omp_csr, m) ? "yes" : "no");

    start = now_seconds();
    int *x = vector;
    for (int i = 0; i < iterations; i++) {
        int *y = csr_multiply(csr, x, m);
        if (i > 0) free(x);
        x = y;
    }
    end = now_seconds();
    printf("CSR serial multiplication took %.6f seconds\n", end - start);

    start = now_seconds();
    int *omp_x = vector;
    for (int i = 0; i < iterations; i++) {
        int *y = csr_multiply_omp(csr, omp_x, m);
        if (i > 0) free(omp_x);
        omp_x = y;
    }
    end = now_seconds();
    printf("CSR parallel multiplication took %.6f seconds\n", end - start);
    printf("Do final vectors results match: %s\n", check_results(x, omp_x, m) ? "yes" : "no");

    start = now_seconds();
    int *dense_x = vector;
    for (int i = 0; i < iterations; i++) {  
        int *y = dense_multiply(array, dense_x, m);
        if (i > 0) free(dense_x);
        dense_x = y;
    }
    end = now_seconds();
    printf("Dense serial multiplication took %.6f seconds\n", end - start);
    printf("Do final vectors results match: %s\n", check_results(x, dense_x, m) ? "yes" : "no");


    start = now_seconds();
    int *dx = vector;
    for (int i = 0; i < iterations; i++) {
        int *y = dense_multiply_omp(array, dx, m);
        if (i > 0) free(dx);
        dx = y;
    }
    end = now_seconds();
    printf("Dense parallel multiplication took %.6f seconds\n", end - start);
    printf("Do final vectors results match: %s\n", check_results(x, dx, m) ? "yes" : "no");

    return 0;
}