#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdbool.h>
#include <pthread.h>

typedef struct task_type task_type;
struct task_type {
    void (*task_func)(void* args);
    void* args;
    void (*cleanup_func)(void* args);
};

typedef struct task_node task_node;
struct task_node {
    task_type task;
    task_node* next_ptr;
};

typedef struct task_queue task_queue;
struct task_queue {
    task_node* front_ptr;
    task_node* back_ptr;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool shutdown;
};

bool tasks_pending(task_queue* q);
task_queue init_task_queue();
void enqueue_task(task_queue* q, task_type t);
task_type dequeue_task(task_queue* q);
void run_task(task_type t);
void free_task_queue(task_queue* q);

#endif // TASK_QUEUE_H
