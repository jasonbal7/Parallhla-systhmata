#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "globals.h"
#include "transactions.h"
#include "workers.h"

void run_with_mutex_cg()
{
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    account_mutex = malloc(sizeof(pthread_mutex_t) * size);
    if (!account_mutex) {
        perror("malloc");
        return;
    }
    for (int i = 0; i < size; i++) {
        pthread_mutex_init(&account_mutex[i], NULL);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_with_mutex_cg, &thread_ids[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }   

    for (int i = 0; i < size; i++) {
        pthread_mutex_destroy(&account_mutex[i]);
    }
    free(account_mutex);
}

void run_with_mutex_fg()
{
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    account_mutex = malloc(sizeof(pthread_mutex_t) * size);
    if (!account_mutex) {
        perror("malloc");
        return;
    }

    for (int i = 0; i < size; i++) {
        pthread_mutex_init(&account_mutex[i], NULL);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_with_mutex_fg, &thread_ids[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }   

    for (int i = 0; i < size; i++) {
        pthread_mutex_destroy(&account_mutex[i]);
    }
    free(account_mutex);
}

void run_with_rwlock_cg()
{
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    account_rwlock = malloc(sizeof(pthread_rwlock_t) * size);
    if (!account_rwlock) {
        perror("malloc");
        return;
    }

    for (int i = 0; i < size; i++) {
        pthread_rwlock_init(&account_rwlock[i], NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_with_rwlock_cg, &thread_ids[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }   

    for (int i = 0; i < size; i++) {
        pthread_rwlock_destroy(&account_rwlock[i]);
    }

    free(account_rwlock);
}

void run_with_rwlock_fg()
{
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    account_rwlock = malloc(sizeof(pthread_rwlock_t) * size);
    if (!account_rwlock) {
        perror("malloc");
        return;
    }

    for (int i = 0; i < size; i++) {
        pthread_rwlock_init(&account_rwlock[i], NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker_with_rwlock_fg, &thread_ids[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }   

    for (int i = 0; i < size; i++) {
        pthread_rwlock_destroy(&account_rwlock[i]);
    }

    free(account_rwlock);
}

double now_seconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void reset_global_vars()
{
    int total_trans = trans_per_thread * num_threads;
    balance_trans = (percentage * total_trans) / 100; 
    money_trans = total_trans - balance_trans;

    generate_random_array(array, size);
}

// ./program <size> <transactions_number_per_thread> <percentage> <lock_type> <num_threads>
int main(int argc, char *argv[])    
{
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <size> <transactions_number_per_thread> <percentage> <lock_type> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    size = atoi(argv[1]);
    trans_per_thread = atoi(argv[2]);
    percentage = atoi(argv[3]);
    lock_type = argv[4];
    num_threads = atoi(argv[5]);

    array = malloc(sizeof(double) * size);
    if (!array) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    int total_trans = trans_per_thread * num_threads;
    balance_trans = (percentage * total_trans) / 100; 
    money_trans = total_trans - balance_trans;
    generate_random_array(array, size);

    printf("Total size: %d\n", size);
    printf("Transactions per thread: %d\n", trans_per_thread);
    printf("Total transactions: %d\n", total_trans);
    printf("Total balance transactions: %d\n", balance_trans);
    printf("Total money transfer transactions: %d\n", money_trans);


    if (strcmp(lock_type, "mutex") == 0) {
        printf("\n=== Running COARSE-GRAINED MUTEX ===\n");
        double start = now_seconds();
        run_with_mutex_cg();
        double end = now_seconds();
        printf("Time taken: %.6f seconds\n", end - start);
        reset_global_vars();

        printf("\n=== Running FINE-GRAINED MUTEX ===\n");
        start = now_seconds();
        run_with_mutex_fg();
        end = now_seconds();
        printf("Time taken: %.6f seconds\n", end - start);
        reset_global_vars();
    }
    else if (strcmp(lock_type, "rwlock") == 0) {
        printf("\n=== Running COARSE-GRAINED RWLOCK ===\n");
        double start = now_seconds();
        run_with_rwlock_cg();
        double end = now_seconds();
        printf("Time taken: %.6f seconds\n", end - start);
        reset_global_vars();

        printf("\n=== Running FINE-GRAINED RWLOCK ===\n");
        start = now_seconds();
        run_with_rwlock_fg();
        end = now_seconds();
        printf("Time taken: %.6f seconds\n", end - start);
        reset_global_vars();
    }
    else {
        fprintf(stderr, "Invalid lock type. Use 'mutex' or 'rwlock'.\n");
        free(array);
        free(account_rwlock);
        return EXIT_FAILURE;
    }

    free(array);
    return 0;
}    