#include "workers.h"
#include "transactions.h"
#include "globals.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>


void *worker_with_mutex_cg(void *arg)
{
    int thread_id = *(int *)arg;
    unsigned int seed = time(NULL) ^ thread_id;

    int my_count = 0;
    int my_sum = 0;

    while (my_count < trans_per_thread) {

        int job = choose_job(&seed);                // 0 for money transfer, 1 for show balance

        pthread_mutex_lock(&counter_mutex);
        if (money_trans == 0 && balance_trans == 0) {
            pthread_mutex_unlock(&counter_mutex);
            break;
        }

        if (job == 0) {
            if (money_trans > 0) {
                int success = money_transfer_transaction(&seed);
                if (success) {
                    money_trans--;
                    my_count++;
                }
            }
            else if (balance_trans > 0) {
                balance_trans--;
                my_sum += show_balance_transaction(&seed);
                my_count++;
            }
        }
        else if (job == 1) {
            if (balance_trans > 0) {
                my_sum += show_balance_transaction(&seed);
                balance_trans--;
                my_count++;
            }
            else if (money_trans > 0) {
                int success = money_transfer_transaction(&seed);
                if (success) {
                    money_trans--;
                    my_count++;
                }
            }
        }
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

void *worker_with_rwlock_cg(void *arg)
{
    int thread_id = *(int *)arg;
    unsigned int seed = time(NULL) ^ thread_id;
    int my_count = 0;
    int my_sum = 0;

    while (my_count < trans_per_thread) {
        int job = choose_job(&seed);

        pthread_rwlock_wrlock(&counter_rwlock);
        if (money_trans == 0 && balance_trans == 0) {
            pthread_rwlock_unlock(&counter_rwlock);
            break;
        }

        if (job == 0) {
            if (money_trans > 0) {
                int success = money_transfer_transaction(&seed);
                if (success) {
                    money_trans--;
                    my_count++;
                }
            }
            else if (balance_trans > 0) {
                balance_trans--;
                my_sum += show_balance_transaction(&seed);
                my_count++;
            }
        }
        else if (job == 1) {
            if (balance_trans > 0) {
                my_sum += show_balance_transaction(&seed);
                balance_trans--;
                my_count++;
            }
            else if (money_trans > 0) {
                int success = money_transfer_transaction(&seed);
                if (success) {
                    money_trans--;
                    my_count++;
                }
            }
        }
        pthread_rwlock_unlock(&counter_rwlock);
    }
    return NULL;
}   


void *worker_with_mutex_fg(void *arg)
{
    int thread_id = *(int *)arg;
    unsigned int seed = time(NULL) ^ thread_id;
    int my_total = 0;
    int my_sum = 0;

    while (my_total < trans_per_thread) {

        int job = choose_job(&seed);
        pthread_mutex_lock(&counter_mutex);

        if (money_trans == 0 && balance_trans == 0) {
            pthread_mutex_unlock(&counter_mutex);
            break;
        }

        int do_money = 0;
        int do_balance = 0;

        if (job == 0) {
            if (money_trans > 0) {
                do_money = 1;
                money_trans--;
            }
            else if (balance_trans > 0) {
                do_balance = 1;
                balance_trans--;
            }
        }
        else if (job == 1) {
            if (balance_trans > 0) {
                do_balance = 1;
                balance_trans--;
            }
            else if (money_trans > 0) {
                do_money = 1;
                money_trans--;
            }
        }
        pthread_mutex_unlock(&counter_mutex);

        if (do_money) {
            int success = money_transfer_transaction_fg(&seed);
            if (success) {
                my_total++;
            }
            else {
                pthread_mutex_lock(&counter_mutex);
                money_trans++;
                pthread_mutex_unlock(&counter_mutex);
            }
        }
        else if (do_balance) {
            my_sum += show_balance_transaction_fg(&seed);
            my_total++;
        }
    }
    return NULL;
}

void *worker_with_rwlock_fg(void *arg)
{
    int thread_id = *(int *)arg; 
    unsigned int seed = time(NULL) ^ thread_id;
    int my_total = 0;
    int my_sum = 0;

    while (my_total < trans_per_thread) {

        int job = choose_job(&seed);
        pthread_rwlock_wrlock(&counter_rwlock);

        if (money_trans == 0 && balance_trans == 0) {
            pthread_rwlock_unlock(&counter_rwlock);
            break;  
        }

        int do_money = 0;
        int do_balance = 0;

        if (job == 0) {
            if (money_trans > 0) {
                do_money = 1;
                money_trans--;
            }
            else if (balance_trans > 0) {
                do_balance = 1;
                balance_trans--;
            }
        }
        else if (job == 1) {
            if (balance_trans > 0) {
                do_balance = 1;
                balance_trans--;
            }
            else if (money_trans > 0) {
                do_money = 1;   
                money_trans--;
            }
        }
        pthread_rwlock_unlock(&counter_rwlock);
        
        if (do_money) {
            int success = money_transfer_transaction_rw_fg(&seed);
            if (success) {
                my_total++;
            }
            else {
                pthread_rwlock_wrlock(&counter_rwlock);
                money_trans++;
                pthread_rwlock_unlock(&counter_rwlock);
            }
        }
        else if (do_balance) {
            my_sum += show_balance_transaction_rw_fg(&seed);
            my_total++;
            
        }
    }
    return NULL;
}