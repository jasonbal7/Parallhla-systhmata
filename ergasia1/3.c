#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct array_stats_s {
    long long int info_array_0;
    long long int info_array_1;
    long long int info_array_2;
    long long int info_array_3;
} array_stats;


struct thread_args {
    int *array;
    long long int size;
    long long int *stats;
};

double now_seconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int *generate_random_array(long long int size)
{
    int *array = malloc(size * sizeof(int));
    if (!array) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (long long int i = 0; i < size; i++) {
        array[i] = rand() % 10;  //for random in [0, 9]
    }

    return array;
}

void *worker(void *arg)
{
    struct thread_args *t_args = arg;

    long long int count = 0;
    for (long long int i = 0; i < t_args->size; ++i) {
        if (t_args->array[i] != 0) {
            count++;
        }
    }
    *(t_args->stats) = count;
    return NULL;
}

void print_array_stats()
{
    printf("Array 0 non-zero count: %lld\n", array_stats.info_array_0);
    printf("Array 1 non-zero count: %lld\n", array_stats.info_array_1);
    printf("Array 2 non-zero count: %lld\n", array_stats.info_array_2);
    printf("Array 3 non-zero count: %lld\n", array_stats.info_array_3);
}

void serial_array_stats(int *array, long long int size, long long int *stats)
{
    long long int count = 0;
    for (long long int i = 0; i < size; ++i) {
        if (array[i] != 0) {
            count++;
        }
    }
    *stats = count;
}

int check_results(struct array_stats_s *parallel, struct array_stats_s *serial)
{
    if (parallel->info_array_0 != serial->info_array_0) return 0;
    if (parallel->info_array_1 != serial->info_array_1) return 0;
    if (parallel->info_array_2 != serial->info_array_2) return 0;
    if (parallel->info_array_3 != serial->info_array_3) return 0;
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long long int size = atoll(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Size must be a positive integer\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    double create_time = now_seconds();
    int *array_1 = generate_random_array(size);
    int *array_2 = generate_random_array(size);
    int *array_3 = generate_random_array(size);
    int *array_4 = generate_random_array(size);
    create_time = now_seconds() - create_time;
    printf("Arrays creation time: %.6f seconds\n", create_time);

    int num_threads = 4;
    pthread_t threads[num_threads];
    struct thread_args args[4];

    args[0] = (struct thread_args){ array_1, size, &array_stats.info_array_0 };
    args[1] = (struct thread_args){ array_2, size, &array_stats.info_array_1 };
    args[2] = (struct thread_args){ array_3, size, &array_stats.info_array_2 };
    args[3] = (struct thread_args){ array_4, size, &array_stats.info_array_3 };

    double start = now_seconds();
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, worker, (void *)&args[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    double end = now_seconds();
    printf("Parallel computation time: %f seconds\n", end - start);
    print_array_stats();

    //serial computation
    struct array_stats_s serial_stats;
    int *arrays[4] = {array_1, array_2, array_3, array_4};
    long long int *serial_count[4] = {
        &serial_stats.info_array_0,
        &serial_stats.info_array_1,
        &serial_stats.info_array_2,
        &serial_stats.info_array_3
    };

    double serial_start = now_seconds();
    for (int i = 0; i < 4; ++i) {
        serial_array_stats(arrays[i], size, serial_count[i]);
    }
    double serial_end = now_seconds();

    printf("Serial computation time: %f seconds\n", serial_end - serial_start);
    

    if (check_results(&array_stats, &serial_stats)) {
        printf("Results match between parallel and serial computations.\n");
    } else {
        printf("Results do NOT match!\n");
    }
}