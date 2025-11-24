#include "globals.h"
#include <stdlib.h>
#include <stdio.h>

int *array = NULL;
int size;

int trans_per_thread;
int balance_trans;
int money_trans;
int percentage;
char *lock_type;
int num_threads;

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *account_mutex = NULL;

pthread_rwlock_t counter_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t *account_rwlock = NULL;