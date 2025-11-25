#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

int value;
int iterations;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
atomic_int value_atomic = 0;

void *worker_with_mutex(void *arg)
{
    (void)arg;
    for (int i = 0; i < iterations; ++i) {
        pthread_mutex_lock(&mutex);
        value += 1;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *worker_with_rwlock(void *arg)
{   
    (void)arg;
    for (int i = 0; i < iterations; ++i) {
        pthread_rwlock_wrlock(&rwlock);
        value += 1;
        pthread_rwlock_unlock(&rwlock);
    }
    return NULL;    
}

void *worker_with_atomic(void *arg) 
{
    (void)arg;
    for (int i = 0; i < iterations; ++i) {
        atomic_fetch_add(&value_atomic, 1);
    }
    return NULL;    
}

double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ./exercise2 <iterations> <threads>
int main(int argc, char *argv[])            
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <iterations> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }  

    iterations = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    value = 0;
    pthread_t threads[num_threads];


    double start = now_seconds();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, worker_with_mutex, NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    double end = now_seconds();
    printf("Final value calculated with mutex lock: %d\n", value);
    printf("Elapsed time with mutex lock: %f seconds\n", end - start);

    value = 0;
    start = now_seconds();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, worker_with_rwlock, NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    end = now_seconds();
    printf("Final value calculated with rwlock: %d\n", value);
    printf("Elapsed time with rwlock: %f seconds\n", end - start);

    value_atomic = 0;
    start = now_seconds();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, worker_with_atomic, NULL);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    end = now_seconds();
    printf("Final value calculated with atomic operations: %d\n", atomic_load(&value_atomic));
    printf("Elapsed time with atomic operations: %f seconds\n", end - start);

    return EXIT_SUCCESS;
}