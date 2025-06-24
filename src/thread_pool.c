#include "../include/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>

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

void thread_pool_init(thread_pool* pool, size_t num_threads)
{
    if (pool == NULL) {
        fprintf(stderr, "Wrong thread pool pointer\n");
        exit(EXIT_FAILURE);
    }
    pool->thread_count = num_threads;
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    pool->queue = init_task_queue();

    for (size_t i = 0; i < num_threads; ++i) {
        pthread_create(&pool->threads[i], NULL, worker_thread, &pool->queue);
    }
}

void thread_pool_submit(thread_pool* pool, task_type task)
{
    if (pool == NULL) {
        fprintf(stderr, "Wrong thread pool pointer\n");
        exit(EXIT_FAILURE);
    }
    enqueue_task(&pool->queue, task);
}

void thread_pool_shutdown(thread_pool* pool)
{
    if (pool == NULL) {
        fprintf(stderr, "Wrong thread pool pointer\n");
        exit(EXIT_FAILURE);
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
}

void cleanup_args_default(void* args)
{
    if (args != NULL) {
        free(args);
    }
}
