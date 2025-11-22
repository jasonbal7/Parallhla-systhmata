#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double *array;
int size;
int trans_per_thread;
int balance_trans;
int money_trans;
int percentage;
char *lock_type;
int num_threads;



pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *account_mutex;

int choose_random_index(int size, unsigned int *seed)
{
    return rand_r(seed) % size;
}

double *generate_random_array(double *array, int size)
{
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 1000;
    }
    return array;
}

void print_array(double *array, int size)
{
    for (int i = 0; i < size; i++) {
        printf("%.2f ", array[i]);
    }
    printf("\n");
}

void money_transfer_transaction(unsigned int *seed)
{
    int i1 = choose_random_index(size, seed);
    int i2 = choose_random_index(size, seed);
    double amount = (double)(rand_r(seed) % 100);

    if (array[i1] >= amount) {
        array[i1] -= amount;
        array[i2] += amount;
    }
    else {
        printf("Money transfer transaction failed: insufficient funds in account %d\n", i1);
    }
}

void money_transfer_transaction_cs(unsigned int *seed)
{
    int i1 = choose_random_index(size, seed);
    int i2 = choose_random_index(size, seed);
    double amount = (double)(rand_r(seed) % 100);

    if (array[i1] >= amount) {
        pthread_mutex_lock(&account_mutex[i1]);
        pthread_mutex_lock(&account_mutex[i2]);
        array[i1] -= amount;
        array[i2] += amount;
        pthread_mutex_unlock(&account_mutex[i2]);
        pthread_mutex_unlock(&account_mutex[i1]);
    }
    else {
        printf("Money transfer transaction failed: insufficient funds in account %d\n", i1);
    }
}

void show_balance_transaction(unsigned int *seed)
{
    int index = choose_random_index(size, seed);
    printf("Account %d balance: %.2f\n", index, array[index]);
}

void show_balance_transaction_cs(unsigned int *seed)
{
    int index = choose_random_index(size, seed);

    pthread_mutex_lock(&account_mutex[index]);
    printf("Account %d balance: %.2f\n", index, array[index]);
    pthread_mutex_unlock(&account_mutex[index]);
}

int choose_job(unsigned int *seed)
{
    int r = rand_r(seed) % 2;     // 0 for money transfer, 1 for show balance
    return r;       
}

void *worker_with_mutex_cg(void *arg)
{
    int thread_id = *(int *)arg;
    unsigned int seed = time(NULL) ^ thread_id;

    int my_count = 0;
    int my_balance_count = 0;

    while (my_count < trans_per_thread) {
        int job = choose_job(&seed);            // 0 for money transfer, 1 for show balance

        if (money_trans == 0 && balance_trans == 0) {
            break;
        }

        if (job == 0) {
            if (money_trans > 0) {
                pthread_mutex_lock(&counter_mutex);
                money_trans--;                          
                money_transfer_transaction(&seed);
                my_count++;
                pthread_mutex_unlock(&counter_mutex);
            }
            else if (balance_trans > 0) {
                pthread_mutex_lock(&counter_mutex);
                balance_trans--;
                show_balance_transaction(&seed);
                my_count++;
                my_balance_count++;
                pthread_mutex_unlock(&counter_mutex);
            }
        }
        else if (job == 1) {
            if (balance_trans > 0) {
                pthread_mutex_lock(&counter_mutex);
                show_balance_transaction(&seed);
                balance_trans--;
                my_count++;
                my_balance_count++;
                pthread_mutex_unlock(&counter_mutex);
            }
            else if (money_trans > 0) {
                pthread_mutex_lock(&counter_mutex);
                money_transfer_transaction(&seed);
                money_trans--;
                my_count++;
                pthread_mutex_unlock(&counter_mutex);
            }
        }
    }

    return NULL;
}

// void *worker_with_rwlock_cg(void *arg)
// {
//     return NULL;
// }   

// void *worker_with_mutex_fg(void *arg)
// {
//     return NULL;
// }

// void *worker_with_rwlock_fg(void *arg)
// {
//     return NULL;
// }

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

    account_mutex = malloc(sizeof(pthread_mutex_t) * size);
    if (!account_mutex) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < size; i++) {
        pthread_mutex_init(&account_mutex[i], NULL);
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

    pthread_t threads[num_threads];
    int thread_ids[num_threads];
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

    free(array);
    return 0;
}    
    




//fine-grained logic 

// 1. Reserve transaction
// pthread_mutex_lock(&counter_lock);
// money_trans--;          // reserve
// pthread_mutex_unlock(&counter_lock);

// // 2. Do transfer on array
// pthread_mutex_lock(&account_mutex[i1]);
// pthread_mutex_lock(&account_mutex[i2]);
// array[i1] -= amount;
// array[i2] += amount;
// pthread_mutex_unlock(&account_mutex[i2]);
// pthread_mutex_unlock(&account_mutex[i1]);