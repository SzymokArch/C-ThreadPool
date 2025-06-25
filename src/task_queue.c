#include "../include/task_queue.h"
#include <asm-generic/errno-base.h>
#include <pthread.h>
#include <stdlib.h>

bool tasks_pending(task_queue* q)
{
    if (q == NULL) {
        return false;
    }
    return q->front_ptr != NULL;
}

int init_task_queue(task_queue* q)
{
    q->front_ptr = NULL;
    q->back_ptr = NULL;
    int mutex_init_retval = pthread_mutex_init(&(q->mutex), NULL);
    if (mutex_init_retval != 0) {
        return mutex_init_retval;
    }
    int cond_init_retval = pthread_cond_init(&(q->cond), NULL);
    if (cond_init_retval != 0) {
        return cond_init_retval;
    }
    return 0;
}

int enqueue_task(task_queue* q, task_type t)
{
    if (q == NULL) {
        return EFAULT;
    }
    task_node* new_node = calloc(1, sizeof(task_node));
    if (new_node == NULL) {
        return ENOMEM;
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
    return 0;
}

task_type dequeue_task(task_queue* q)
{
    if (q == NULL) {
        return (task_type) { 0 }; // return empty task
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

int run_task(task_type t)
{
    if (t.task_func == NULL) {
        return EFAULT;
    }
    t.task_func(t.args);
    if (t.cleanup_func) {
        t.cleanup_func(t.args);
    }
    return 0;
}

int free_task_queue(task_queue* q)
{
    if (q == NULL) {
        return EFAULT;
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
    return 0;
}
