#ifndef GLOBALS_H
#define GLOBALS_H

#include <pthread.h>

extern int *array;
extern int size;
extern int trans_per_thread;
extern int balance_trans;
extern int money_trans;
extern int percentage;
extern char *lock_type;
extern int num_threads;

extern pthread_mutex_t counter_mutex;
extern pthread_mutex_t *account_mutex;

extern pthread_rwlock_t counter_rwlock;
extern pthread_rwlock_t *account_rwlock;

#endif