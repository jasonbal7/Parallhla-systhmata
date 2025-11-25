#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <pthread.h>

int choose_random_index(int size, unsigned int *seed);
int *generate_random_array(int *array, int size);
void print_array(int *array, int size);
int choose_job(unsigned int *seed);

int money_transfer_transaction(unsigned int *seed);
int money_transfer_transaction_fg(unsigned int *seed);
int money_transfer_transaction_rw_fg(unsigned int *seed);

int show_balance_transaction(unsigned int *seed);
int show_balance_transaction_fg(unsigned int *seed);
int show_balance_transaction_rw_fg(unsigned int *seed);

#endif