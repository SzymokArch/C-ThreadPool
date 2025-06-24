#include "../include/task_queue.h"
#include <stdio.h>
#include <stdlib.h>

bool tasks_pending(task_queue* q)
{
    if (q == NULL) {
        fprintf(stderr, "Invalid queue address\n");
        exit(EXIT_FAILURE);
    }
    return q->front_ptr != NULL;
}

task_queue init_task_queue(void)
{
    task_queue q = { .front_ptr = NULL,
                     .back_ptr = NULL,
                     .mutex = PTHREAD_MUTEX_INITIALIZER,
                     .cond = PTHREAD_COND_INITIALIZER,
                     .shutdown = false };
    return q;
}

void enqueue_task(task_queue* q, task_type t)
{
    if (q == NULL) {
        fprintf(stderr, "Invalid queue\n");
        exit(EXIT_FAILURE);
    }
    task_node* new_node = calloc(1, sizeof(task_node));
    if (new_node == NULL) {
        fprintf(stderr, "Allocation failure\n");
        exit(EXIT_FAILURE);
    }
    new_node->task = t;

    pthread_mutex_lock(&q->mutex);
    if (q->back_ptr == NULL) {
        q->front_ptr = q->back_ptr = new_node;
    }
    else {
        q->back_ptr->next_ptr = new_node;
        q->back_ptr = new_node;
    }
    pthread_cond_signal(&q->cond); // wake up one waiting worker
    pthread_mutex_unlock(&q->mutex);
}

task_type dequeue_task(task_queue* q)
{
    if (q == NULL) {
        fprintf(stderr, "Invalid queue\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&q->mutex);
    while (!q->front_ptr && !q->shutdown) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (q->shutdown && !q->front_ptr) {
        pthread_mutex_unlock(&q->mutex);
        task_type empty = { 0 }; // return empty task
        return empty;
    }

    task_node* node = q->front_ptr;
    task_type t = node->task;
    q->front_ptr = node->next_ptr;
    if (!q->front_ptr) {
        q->back_ptr = NULL;
    }
    free(node);

    pthread_mutex_unlock(&q->mutex);
    return t;
}

void run_task(task_type t)
{
    if (t.task_func == NULL) {
        return; // gracefully ignore empty/shutdown task
    }
    t.task_func(t.args);
    if (t.cleanup_func) {
        t.cleanup_func(t.args);
    }
}

void free_task_queue(task_queue* q)
{
    if (q == NULL) {
        fprintf(stderr, "Wrong task queue pointer\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&q->mutex);
    task_node* current = q->front_ptr;
    while (current) {
        task_node* next = current->next_ptr;
        if (current->task.cleanup_func) {
            current->task.cleanup_func(current->task.args);
        }
        free(current);
        current = next;
    }
    q->front_ptr = NULL;
    q->back_ptr = NULL;
    pthread_mutex_unlock(&q->mutex);

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}
