#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "task_queue.h"

void* worker_thread(void* arg);

typedef struct thread_pool {
    pthread_t* threads;
    size_t thread_count;
    task_queue queue;
} thread_pool;

void thread_pool_init(thread_pool* pool, size_t num_threads);
void thread_pool_submit(thread_pool* pool, task_type task);
void thread_pool_shutdown(thread_pool* pool);

void cleanup_args_default(void* args);

#define NO_CLEANUP NULL

#endif // THREAD_POOL_H
