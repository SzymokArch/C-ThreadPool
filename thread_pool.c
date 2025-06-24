#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

void* worker_thread(void* arg)
{
    if (arg == NULL) {
        fprintf(stderr, "Wrong task queue pointer\n");
        exit(EXIT_FAILURE);
    }
    task_queue* q = (task_queue*)arg;
    while (true) {
        pthread_mutex_lock(&q->mutex);

        while (!q->front_ptr && !q->shutdown) {
            // Wait for a task or shutdown signal
            pthread_cond_wait(&q->cond, &q->mutex);
        }

        if (q->shutdown && !q->front_ptr) {
            pthread_mutex_unlock(&q->mutex);
            break; // exit thread loop
        }

        // dequeue task safely
        task_node* node = q->front_ptr;
        task_type t = node->task;
        q->front_ptr = node->next_ptr;
        if (q->front_ptr == NULL) {
            q->back_ptr = NULL;
        }
        free(node);

        pthread_mutex_unlock(&q->mutex);

        // run task outside lock
        if (t.task_func) {
            t.task_func(t.args);
        }
        if (t.cleanup_func) {
            t.cleanup_func(t.args);
        }
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
