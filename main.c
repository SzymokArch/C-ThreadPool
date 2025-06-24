#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sched.h>
#include "thread_pool.h"

typedef struct {
    int id;
    _Atomic int* completed_tasks;
} task_args;

void example_task(void* arg)
{
    task_args* data = (task_args*)arg;
    printf("Task %d is running in thread %ld\n", data->id, pthread_self());
    sleep(1); // simulate work
    atomic_fetch_add(data->completed_tasks, 1);
}

void example_task_nocleanup([[maybe_unused]] void* arg)
{
    printf("No parameter task is running in thread %ld\n", pthread_self());
    sleep(1);
}

int main(void)
{
    _Atomic int completed_tasks = 0;

    long system_threads = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Available threads: %ld\n", system_threads);

    thread_pool pool;
    thread_pool_init(&pool, system_threads);

    for (int i = 0; i < 10; ++i) {
        task_args* args = malloc(sizeof(task_args));
        if (!args) {
            fprintf(stderr, "Allocation failure\n");
            exit(EXIT_FAILURE);
        }
        args->id = i;
        args->completed_tasks = &completed_tasks;

        task_type t = { .task_func = example_task,
                        .args = args,
                        .cleanup_func = cleanup_args_default };

        thread_pool_submit(&pool, t);
    }
    while (atomic_load(&completed_tasks) != 10) {
        sched_yield();
    }
    printf("Tasks completed!\n");

    for (int i = 0; i < 10; ++i) {
        task_type t = { .task_func = example_task_nocleanup,
                        .args = NULL,
                        .cleanup_func = NO_CLEANUP };
        thread_pool_submit(&pool, t);
    }

    thread_pool_shutdown(&pool);
    printf("All tasks complete. Pool shut down.\n");

    return 0;
}
