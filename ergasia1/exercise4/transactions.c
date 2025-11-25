#include "transactions.h"
#include "globals.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int choose_random_index(int size, unsigned int *seed)
{
    return rand_r(seed) % size;
}

int *generate_random_array(int *array, int size)
{
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 1000;
    }
    return array;
}

void print_array(int *array, int size)
{
    for (int i = 0; i < size; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

int choose_job(unsigned int *seed)
{
    int r = rand_r(seed) % 100;     // 0 for money transfer, 1 for show balance
    if (r < percentage) return 1;
    else return 0;
}

int money_transfer_transaction(unsigned int *seed)
{
    int i1 = choose_random_index(size, seed);
    int i2 = choose_random_index(size, seed);
    double amount = (double)(rand_r(seed) % 100);

    if (array[i1] >= amount) {
        array[i1] -= amount;
        array[i2] += amount;
        return 1;
    }
    //printf("Money transfer transaction failed: insufficient funds in account %d\n", i1);
    return 0;
}

int money_transfer_transaction_fg(unsigned int *seed)
{
    int i1 = choose_random_index(size, seed);
    int i2 = choose_random_index(size, seed);
    double amount = (double)(rand_r(seed) % 100);

    if (i1 == i2) {
        return 0;
    }

    int first = i1 < i2 ? i1 : i2;
    int second = i1 < i2 ? i2 : i1;

    pthread_mutex_lock(&account_mutex[first]);
    pthread_mutex_lock(&account_mutex[second]);

    if (array[i1] >= amount) {
        array[i1] -= amount;
        array[i2] += amount;
        pthread_mutex_unlock(&account_mutex[second]);
        pthread_mutex_unlock(&account_mutex[first]);
        return 1;
    }

    pthread_mutex_unlock(&account_mutex[second]);
    pthread_mutex_unlock(&account_mutex[first]);
    //printf("Money transfer transaction failed: insufficient funds in account %d\n", i1);
    return 0;
}

int money_transfer_transaction_rw_fg(unsigned int *seed)
{
    int i1 = choose_random_index(size, seed);
    int i2 = choose_random_index(size, seed);
    double amount = (double)(rand_r(seed) % 100);

    if (i1 == i2) {
        return 0;
    }

    int first = i1 < i2 ? i1 : i2;
    int second = i1 < i2 ? i2 : i1;

    pthread_rwlock_wrlock(&account_rwlock[first]);
    pthread_rwlock_wrlock(&account_rwlock[second]);

    if (array[i1] >= amount) {
        array[i1] -= amount;
        array[i2] += amount;
        pthread_rwlock_unlock(&account_rwlock[second]);
        pthread_rwlock_unlock(&account_rwlock[first]);
        return 1;
    }

    pthread_rwlock_unlock(&account_rwlock[second]);
    pthread_rwlock_unlock(&account_rwlock[first]);
    //printf("Money transfer transaction failed: insufficient funds in account %d\n", i1);
    return 0;
}

int show_balance_transaction(unsigned int *seed)
{
    int index = choose_random_index(size, seed);
    //usleep(100);
    return array[index];
}

int show_balance_transaction_fg(unsigned int *seed)
{
    int index = choose_random_index(size, seed);

    pthread_mutex_lock(&account_mutex[index]);
    int balance = array[index];
    pthread_mutex_unlock(&account_mutex[index]);

    //usleep(100);
    
    return balance;
}

int show_balance_transaction_rw_fg(unsigned int *seed)
{
    int index = choose_random_index(size, seed);

    pthread_rwlock_rdlock(&account_rwlock[index]);
    int balance = array[index];
    pthread_rwlock_unlock(&account_rwlock[index]);

    //usleep(100);

    return balance;
}