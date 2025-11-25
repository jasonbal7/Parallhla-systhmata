#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

pthread_barrier_t barrier;

typedef struct {
    long i;
}thread_data_t;

void* f(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;

    for(long i = 0; i < data->i; i++){
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}

double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <threads> <iterations>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nThreads = atoi(argv[1]);
    int i = atol(argv[2]);

    pthread_t threads[nThreads];
    thread_data_t data[nThreads];

    pthread_barrier_init(&barrier, NULL, nThreads);

    double start = now_sec();

    for(int t = 0; t < nThreads; t++) {
        data[t].i = i;
        if(pthread_create(&threads[t], NULL, f, &data[t]) != 0){
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for(int t = 0; t < nThreads; t++) {
        pthread_join(threads[t], NULL);
    }

    double end = now_sec();
    printf("[1] Pthreads_barrier with %d threads took %.3f seconds\n", nThreads, end - start);

    pthread_barrier_destroy(&barrier);
    return 0;
}