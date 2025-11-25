#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


// Sense-reversal centralized barrier.

typedef struct {
    pthread_mutex_t lock;
    volatile bool sense;
    int count;
    int nThreads;
} barrier_t;

void barrier_init(barrier_t* b, int nThreads) {
    b->nThreads = nThreads;
    b->count = 0;
    b->sense = 0;

    pthread_mutex_init(&b->lock, NULL);
}

void barrier_wait(barrier_t* b, bool* localSense) {
    
    *localSense = !(*localSense);

    pthread_mutex_lock(&b->lock);
    b->count++;
    if(b->count == b->nThreads) {
        b->count = 0;
        b->sense = *localSense;
        pthread_mutex_unlock(&b->lock);
    }
    else {
        pthread_mutex_unlock(&b->lock);

        while(b->sense != *localSense);
    }
}

void barrier_destroy(barrier_t* b) {
    pthread_mutex_destroy(&b->lock);
}


typedef struct {
    long i;
    bool localSense;
}thread_data_t;

barrier_t barrier;

void* f(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->localSense = 0;

    for(long i = 0; i < data->i; i++){
        barrier_wait(&barrier, &data->localSense);
    }
    return NULL;
}

double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}


int main(int argc, char* argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <threads> <iterations>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nThreads = atoi(argv[1]);
    int i = atol(argv[2]);

    pthread_t threads[nThreads];
    thread_data_t data[nThreads];

    barrier_init(&barrier, nThreads);

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
    printf("[3] Sense-reversal Barrier with %d threads took %.3f seconds\n", nThreads, end - start);    

    barrier_destroy(&barrier);
    return 0;
}