#include "../include/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <asm-generic/errno-base.h>

void* worker_thread(void* arg)
{
    task_queue* q = (task_queue*)arg;
    while (true) {
        task_type t = dequeue_task(q);
        if (t.task_func == NULL && q->shutdown) {
            break;
        }
        run_task(t);
    }
    return NULL;
}

int thread_pool_init(thread_pool* pool, size_t num_threads)
{
    if (pool == NULL) {
        return EFAULT;
    }
    pool->thread_count = num_threads;
    pool->threads = calloc(num_threads, sizeof(pthread_t));
    if (pool->threads == NULL) {
        return ENOMEM;
    }
    int init_task_queue_retval = init_task_queue(&(pool->queue));
    if (init_task_queue_retval != 0) {
        return init_task_queue_retval;
    }

    for (size_t i = 0; i < num_threads; ++i) {
        pthread_create(&(pool->threads[i]), NULL, worker_thread, &pool->queue);
    }
    return 0;
}

int thread_pool_submit(thread_pool* pool, task_type task)
{
    if (pool == NULL) {
        return EFAULT;
    }
    enqueue_task(&pool->queue, task);
    return 0;
}

int thread_pool_shutdown(thread_pool* pool)
{
    if (pool == NULL) {
        return EFAULT;
    }
    pthread_mutex_lock(&pool->queue.mutex);
    pool->queue.shutdown = true;
    pthread_cond_broadcast(&pool->queue.cond); // wake all workers
    pthread_mutex_unlock(&pool->queue.mutex);

    for (size_t i = 0; i < pool->thread_count; ++i) {
        pthread_join(pool->threads[i], NULL);
    }
    free(pool->threads);
    free_task_queue(&pool->queue);
    return 0;
}

void cleanup_args_default(void* args)
{
    if (args != NULL) {
        free(args);
    }
}
