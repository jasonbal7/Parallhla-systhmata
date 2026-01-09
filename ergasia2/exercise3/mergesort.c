#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>

// Merge 2 sorted parts of array A. 
void merge(int* A, int start_l, int end_l, int end_r, int* temp) { 
    int i = start_l;    // start_l/end_l is the starting/ending index in the left array. 
    int j = end_l + 1;  // end_r is the ending index in the right array.
    int k = start_l;

    while(i <= end_l && j <= end_r) {
        if(A[i] <= A[j]) {
            temp[k++] = A[i++];
        }
        else {
            temp[k++] = A[j++];
        }
    }

    while(i <= end_l) {   // Add remaining values of left array in the end, if any.
        temp[k++] = A[i++];
    }
    while(j <= end_r) {  // Add remaining values of right array in the end, if any.
        temp[k++] = A[j++];
    }

    for(i = start_l; i <= end_r; i++) {
        A[i] = temp[i];
    }
}


void mergesort_serial(int* A, int left, int right, int* temp) {
    if(left >= right){
        return;
    }

    int mid = (left + right) / 2;
    mergesort_serial(A, left, mid, temp);
    mergesort_serial(A, mid + 1, right, temp);
    merge(A, left, mid, right, temp);
}


void mergesort_parallel(int* A, int left, int right, int* temp) {
    if(left >= right) {
        return;
    }

    int mid = (left + right) / 2;

    #pragma omp task if (right - left > 10000)
    mergesort_parallel(A, left, mid, temp);

    #pragma omp task if (right - left > 10000)
    mergesort_parallel(A, mid + 1, right, temp);

    #pragma omp taskwait
    merge(A, left, mid, right, temp);
}


// Check if the array is sorted.
bool is_sorted(int* A, int n) {
    for(int i = 1; i < n; i++) {
        if(A[i] < A[i-1]) {
            return 0;
        }
    }
    return 1;
}

//Print array.
// void print_array(int* A, int n) {
//     for(int i = 0; i < n; i++) {
//         printf("[%d] ", A[i]);
//     }
// }


int main(int argc, char* argv[]) {
    if(argc != 4) {
        printf("Usage: %s <N> <s or p> <nThreads> \n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    char algorithm = argv[2][0];
    int nThreads = atoi(argv[3]);

    int* A = malloc(N * sizeof(int));
    int* temp = malloc(N * sizeof(int));
    
    if(!A || !temp) {
        printf("malloc failed \n");
        return 1;
    }

    srand(2200195);
    for(int i = 0; i < N; i++){    // Create "random" int array.
        A[i] = rand();
    }

    double start, end;

    if(algorithm == 's') {          // Select algorithm.
        start = omp_get_wtime();
        mergesort_serial(A, 0, N-1, temp);
        end = omp_get_wtime();
    }

    else if(algorithm == 'p') {
        start = omp_get_wtime();
        #pragma omp parallel num_threads(nThreads)
        {
            #pragma omp single           // Only one thread in the initial call.
            mergesort_parallel(A, 0, N-1, temp);
        }
        end = omp_get_wtime();
    }

    else {
        printf("No valid algorithm must use 's' for serial or 'p' for parallel \n");
        return 1;
    }

    if(is_sorted(A, N)) {
        printf("Successful sorting! \n");
        printf("Time of %c mergesort algorithm for %d ints with %d threads is %.3f seconds.\n", algorithm, N, nThreads, end - start);
    }
    else {
        printf("Error in sorting! \n");
    }

    free(A);
    free(temp);
    return 0;
}