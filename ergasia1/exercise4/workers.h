#ifndef WORKERS_H
#define WORKERS_H

void *worker_with_mutex_cg(void *arg);
void *worker_with_mutex_fg(void *arg);
void *worker_with_rwlock_cg(void *arg);
void *worker_with_rwlock_fg(void *arg);

#endif