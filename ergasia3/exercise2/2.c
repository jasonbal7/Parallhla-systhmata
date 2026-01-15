#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

typedef struct {
    int* values;
    int* col_index;
    int* row_ptr;
    int nnz;
    int rows;
} CSRMatrix;


double drand() {
    return (double)rand() / RAND_MAX;
}
int* create_sparse_array(int n, double sparsity) {  // n x n. Flattened matrix.
    int* A = (int*)malloc(n * n * sizeof(int));
    if(!A) {
        fprintf(stderr, "Malloc failed.\n");
        return NULL;
    }
    for(int i = 0; i < n * n; i++) {
        if(drand() < sparsity) {
            A[i] = 0;
        }
        else {
            A[i] = rand() % 99 + 1;
        }
    }
    return A;
}

int* create_vector(int n) {        // Create random vector.
    int* vector = (int*)malloc(n * sizeof(int));
    if(!vector){
        fprintf(stderr, "Malloc failed\n");
        return NULL;
    }
    for(int i = 0; i < n; i++) {
        vector[i] = rand() % 99 + 1;
    }
    return vector;
}

CSRMatrix convert_to_csr(int* array, int n) {

    CSRMatrix csr;
    int nnz = 0;

    for(int i = 0; i < n * n; i++) {     // Count non zero.
        if(array[i] != 0) nnz++;
    }

    csr.nnz = nnz;
    csr.rows = n;
    csr.values = (int*)malloc(nnz * sizeof(int));    // Create CSR arrays.
    csr.col_index = (int*)malloc(nnz * sizeof(int));
    csr.row_ptr = (int*)malloc((n + 1) * sizeof(int));

    int k = 0;
    csr.row_ptr[0] = 0;
    for(int i = 0; i < n; i++){      // Fill CSR arrays.
        for(int j = 0; j < n; j++) {
            if(array[i * n + j] != 0) {
                csr.values[k] = array[i * n + j];
                csr.col_index[k] = j;
                k++;
            }
        }
        csr.row_ptr[i+1] = k;
    }
    return csr;
}


void csr_multiply_local(CSRMatrix csr, int* x, int* y) {
    for(int i = 0; i < csr.rows; i++) {
        int sum = 0;
        int row_start = csr.row_ptr[i];
        int row_end   = csr.row_ptr[i+1];
        for(int j = row_start; j < row_end; j++) {
            sum += csr.values[j] * x[csr.col_index[j]];
        }
        y[i] = sum;
    }
}

void dense_multiply_local(int* local_matrix, int* x, int* y, int rows, int cols) {
    for(int i = 0; i < rows; i++) {
        int sum = 0;
        for(int j = 0; j < cols; j++) {
            sum += local_matrix[i * cols + j] * x[j];
        }
        y[i] = sum;
    }
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(argc != 4) {
        if(rank == 0) {
            fprintf(stderr, "Usage: %s <n> <sparsity> <iterations> \n", argv[0]);   }
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);
    double sparsity = atof(argv[2]);
    int iterations = atoi(argv[3]);

    double time_construct = 0;
    double time_comm_s, time_comm_e;
    double time_calculation_s, time_calculation_e;

    double time_dense_comm_s, time_dense_comm_e;
    double time_dense_calc_s, time_dense_calc_e;

    int* global_matrix = NULL;
    int* global_vector = NULL;
    CSRMatrix global_csr;


    if(rank == 0) {   // Create and initialize matrix and vectors.
        srand(time(NULL));
        global_matrix = create_sparse_array(n, sparsity);
        global_vector = create_vector(n);

        double t1 = MPI_Wtime();
        global_csr = convert_to_csr(global_matrix, n);
        time_construct = MPI_Wtime() - t1;
    }

    int rows_per_process = n / size;
    int rest = n % size;

    int *sendcounts_rows = malloc(size * sizeof(int));
    int *offset_rows = malloc(size * sizeof(int));

    int curr_offset = 0;
    for(int i = 0; i < size; i++) {
        sendcounts_rows[i] = rows_per_process + (i < rest ? 1 : 0);
        offset_rows[i] = curr_offset;
        curr_offset += sendcounts_rows[i];
    }
    int local_rows = sendcounts_rows[rank];

    int* x = (int*)malloc(n * sizeof(int));      // Broadcast the vector to all processes.
    if(rank == 0){
        memcpy(x, global_vector, n * sizeof(int));
    }
    MPI_Bcast(x, n, MPI_INT, 0, MPI_COMM_WORLD);

    int* local_row_ptr = (int*)malloc((local_rows + 1) * sizeof(int));
    int* nnz_counts = malloc(size * sizeof(int));
    int* nnz_offset = malloc(size * sizeof(int));
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        time_comm_s = MPI_Wtime();
    }


    MPI_Scatterv(rank == 0 ? global_csr.row_ptr : NULL, sendcounts_rows, offset_rows, MPI_INT,  // Send rows' start indexes.
                local_row_ptr, local_rows, MPI_INT, 0, MPI_COMM_WORLD);


    int local_nnz;

    if(rank == 0) {       // Compute non-zeros per process.
        for(int i = 0; i < size; i++) {
            int row_s = offset_rows[i];
            int row_e = row_s + sendcounts_rows[i];
            nnz_counts[i] = global_csr.row_ptr[row_e] - global_csr.row_ptr[row_s];
            nnz_offset[i] = global_csr.row_ptr[row_s];
        }
    }
    MPI_Scatter(nnz_counts, 1, MPI_INT, &local_nnz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    CSRMatrix local_csr;          // Create local csr, send values and column indices.
    local_csr.rows = local_rows;
    local_csr.nnz = local_nnz;
    local_csr.row_ptr = local_row_ptr;
    local_csr.values = (int*)malloc(local_nnz * sizeof(int));
    local_csr.col_index = (int*)malloc(local_nnz * sizeof(int));
    MPI_Scatterv(rank == 0 ? global_csr.values : NULL, nnz_counts, nnz_offset, MPI_INT, 
                local_csr.values, local_nnz, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(rank == 0 ? global_csr.col_index : NULL, nnz_counts, nnz_offset, MPI_INT,
                local_csr.col_index, local_nnz, MPI_INT, 0, MPI_COMM_WORLD);

    int local_offset = local_row_ptr[0];
    for(int i = 0; i < local_rows; i++) {    // Adjust row_ptr to local indeces.
        local_csr.row_ptr[i] -= local_offset;
    }
    local_csr.row_ptr[local_rows] = local_nnz;

    if(rank == 0) {
        time_comm_e = MPI_Wtime();
    }


    int* local_y = (int*)malloc(local_rows * sizeof(int));
    MPI_Barrier(MPI_COMM_WORLD);

    time_calculation_s = MPI_Wtime();
    for(int i = 0; i < iterations; i++) {
        csr_multiply_local(local_csr, x, local_y);
        MPI_Allgatherv(local_y, local_rows, MPI_INT,       // y = x in the next iteration.
                       x, sendcounts_rows, offset_rows, MPI_INT, MPI_COMM_WORLD);
    }
    time_calculation_e = MPI_Wtime();

    if(rank == 0) {
        double communication_time = time_comm_e - time_comm_s;
        double calculation_time = time_calculation_e - time_calculation_s;

        printf("CSR with %d processes and %d iterations.\n", size, iterations);
        printf("Final result vector (CRS):\n");
        for(int i = 0; i < n; i++){
            printf("%d ", x[i]);
        }
        printf("\nConstruction Time CSR   = %f sec\n", time_construct);
        printf("Communication Time CSR    = %f sec\n", communication_time);
        printf("Calculation Time CSR      = %f sec\n", calculation_time);
        printf("Total CSR time            = %f sec\n", time_construct + communication_time + calculation_time);
    }


    if(rank == 0) {
        memcpy(x, global_vector, n * sizeof(int));
    }
    MPI_Bcast(x, n, MPI_INT, 0, MPI_COMM_WORLD);

    int* sendcounts_dense = malloc(size * sizeof(int));
    int* offset_dense = malloc(size * sizeof(int));
    int* local_dense_matrix = malloc(local_rows * n * sizeof(int));

    for(int i = 0; i < size; i++) {
        sendcounts_dense[i] = sendcounts_rows[i] * n;
        offset_dense[i] = offset_rows[i] * n;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        time_dense_comm_s = MPI_Wtime();
    }
    MPI_Scatterv(global_matrix, sendcounts_dense, offset_dense, MPI_INT,
                 local_dense_matrix, local_rows * n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 0) {
        time_dense_comm_e = MPI_Wtime();
    }

    time_dense_calc_s = MPI_Wtime();
    for(int i = 0; i < iterations; i++) {     // Dense multiply.
        dense_multiply_local(local_dense_matrix, x, local_y, local_rows, n);
        MPI_Allgatherv(local_y, local_rows, MPI_INT, 
                       x, sendcounts_rows, offset_rows, MPI_INT, MPI_COMM_WORLD);
    }
    time_dense_calc_e = MPI_Wtime();

    if(rank == 0) {
        double dense_calc_time = time_dense_calc_e - time_dense_calc_s;
        printf("Calculation Time Dense    = %f sec\n", dense_calc_time);
        printf("Total Dense Time          = %f sec\n", time_dense_comm_e - time_dense_comm_s + dense_calc_time);
        printf("----------------------\n\n\n");
    }


    free(x);
    free(local_y);
    free(local_csr.values);
    free(local_csr.col_index);
    free(local_csr.row_ptr);
    free(sendcounts_rows);
    free(offset_rows);
    free(nnz_counts);
    free(nnz_offset);
    free(sendcounts_dense);
    free(offset_dense);
    free(local_dense_matrix);

    if(rank == 0) {
        free(global_matrix);
        free(global_vector);
        free(global_csr.values);
        free(global_csr.col_index);
        free(global_csr.row_ptr);
    }

    MPI_Finalize();
    return 0;
    
}