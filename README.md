# C-ThreadPool
A simple thread pool implementation in C. 
Tested under `gcc 15.1.1` and `clang 20.1.6` with `-Wall -Wextra -Wpedantic` flags on GNU/Linux. The code doesn't emit any errors or warnings (*excluding `-Wc23-extensions` in the provided example `main.c`*).
# Actually using the thread pool
Clone the repo and place all the source files (*excluding the example `main.c` file*) inside the directory in which you want to use the thread pool.
Inside your source files you'll want to write
```c
#include "thread_pool.h"
```
Currently the build command I'm using is this:
```bash 
clang main.c task_queue.c thread_pool.c -lm -lpthread -Wall -Wextra -Wpedantic -o out 
```
and then to run the example
```bash
./out
```
You can substitute the `main.c` with whatever you want. In the future I'll probably make some fancy Makefile to make compilation less of a hassle.
## Important stuff here!
The core of this thread pool implementation is the `task_type` structure. The structure contains two function pointers and a void pointer pointing to the function's parameters. The definition of the parameters is left to the programmer. 
```c
typedef struct task_type task_type;
struct task_type {
    void (*task_func)(void* args);
    void* args;
    void (*cleanup_func)(void* args);
};
```
Since we want the tasks to accepts all the different types of arguments the actual parameters of the function pointers need to be a void pointer, also we have no way to handle return types so the functions can't return any value.
This implementation assumes that the programmer will make a struct containing the arguments to the function you want to submit to the pool and manually mange the struct's memory.
For example:
```c
typedef struct {
	int id;
	_Atomic int* completed_tasks;
} task_args;
```
And then allocating the space for `args` before submitting the task to the queue, for example:
```c
task_args* args = malloc(sizeof(task_args));
if (!args) {
	fprintf(stderr, "Allocation failure\n");
	exit(EXIT_FAILURE);
}
args->id = i; // loop index
args->completed_tasks = &completed_tasks; // pointer to _Atomic int that was declared in the beginning of main.c
```
Example task functions:
```c
void example_task(void* arg)
{
	task_args* data = (task_args*)arg;
	printf("Task %d is running in thread %ld\n", data->id, pthread_self());
	sleep(1); // simulate work
	atomic_fetch_add(data->completed_tasks, 1); // add one to the completed tasks counter
}
```
```c
void example_task_nocleanup([[maybe_unused]] void* arg)
{
	printf("No parameter task is running in thread %ld\n", pthread_self());
	sleep(1);
}
```
(*Note the* `[[maybe_unused]]` *attribute, without it the code will emit a warning that the* `arg` *variable is not used, which is obviously true, since the pointer is* `NULL` *and there's no point in using this variable, but the function **has to** have a void pointer as it's parameter so that the function is compatible with the task function pointer. When creating tasks without parameters feel free to ignore this warning if you absolutely can't use C23 extensions*)

After the task function is completed the cleanup function will be called. Most of the time the function will only be needed to free the `args` pointer, there is no need to define this manually since there is a function that does just that
```c
void cleanup_args_default(void* args)
```
And then when creating the task you can for example do:
```c
task_type t = { .task_func = example_task,
		.args = args, // args pointer allocated earlier
		.cleanup_func = cleanup_args_default };
```
If you don't want the task function to take any parameters, you can pass `NULL` into the `.args` field and the `NO_CLEANUP` constant inside the `.cleanup_func` field, like so:
```c
task_type t = { .task_func = example_task_nocleanup,
		.args = NULL,
		.cleanup_func = NO_CLEANUP };
```
. If you want the cleanup function to do more complex stuff than freeing the space occupied by `args` or doing nothing, you can define your own function and pass it into the `.cleanup_func` field. 

When you finally want to submit the task you defined to the thread pool you just need to call the function 
```c
thread_pool_submit(thread_pool* pool, task_type task)
``` 
## Potentially useful stuff
### Atomic variables
You could probable see in the provided examples the usage of the `_Atomic` keyword to implement a completed tasks counter. The `_Atomic` keyword in C is used to declare a variable as atomic, meaning that reads, writes, and some operations on that variable are guaranteed to be performed without interference from other threads. For example, if one thread is modifying the variable, another thread won’t see a half-written value.  Simple types like `int` are not necessarily safe to read/write from multiple threads simultaneously. Compilers and CPUs can reorder operations for optimization, which can break thread-safety unless told not to. `_Atomic` keyword from the `stdatomic.h` header is really easy and simple to use. You can read about the [details](https://en.cppreference.com/w/c/atomic.html) if you'd like to learn more.
### Getting the number of logical cores
If you don't know what number of threads you should use when initializing you can't go wrong with the amount of logical cores you have on your system (*For example 16 on an 8 core CPU with hyperthreading*). You don't have to know the number from the top your head. There's a useful function in the `unistd.h` header that can be used to retrieve system configuration at run time. I'm talking about the `sysconf(int name)` function. When passing the constant `_SC_NPROCESSORS_ONLN` the functions returns the number of processors currently online (available). Example:
```c
long system_threads = sysconf(_SC_NPROCESSORS_ONLN);
printf("Available threads: %ld\n", system_threads);
```
Obviously it only works on POSIX systems like GNU/Linux for example. This entire code won't work on Windows and may God have mercy on your soul if you program C on Windows without WSL.
### Waiting for the tasks to finish
You may have observed the presence of this loop in my example code:
```c
while (atomic_load(&completed_tasks) != 10) {
	sched_yield();
}
```
And pondered what the hell it does and most importantly what even is this `sched_yield()` function. I already explained the concept of `_Atomic` variables. You can't just perform normal read/write operations on these variables to retain their thread safety. That is why I call upon the `atomic_load()` function to load the variable so that I can safely use it, in this example I just check whether the 10 enqued tasks finished. I don't the control flow of the program to continue until these tasks have finished executing. That's why there is a loop that waits until the `completed_tasks` counter reaches 10. That's basically all that the `sched_yield()` is doing here. Since we can't have an empty loop (*The all-knowing compiler will just remove it since it doesn't do anything*). `sched_yield()` is a system call that allows a thread to voluntarily relinquish the CPU, giving other threads of equal priority a chance to run. In the context of a thread pool or any multi-threaded application, it's particularly useful when a thread is waiting for a condition to be met without blocking, such as when polling for task completion.
By calling `sched_yield()` inside a loop, the thread avoids consuming 100% CPU time in a tight busy-wait. This improves CPU efficiency, promotes fair scheduling, and allows other worker threads to make progress—leading to better overall performance in concurrent applications. 
You can achieve a similar effect by calling `sleep(0)`.
Both `sched_yield()` and `sleep(0)` are used to voluntarily give up the CPU, but they behave slightly differently:
-   `sched_yield()`: Yields the processor _only_ to other threads of **equal or higher priority** that are ready to run. It does **not** sleep or delay; it just says, "If someone else is ready, let them run."
-   `sleep(0)`: On most Unix-like systems, this is interpreted as a no-op or as yielding to **any other runnable thread**, similar to `sched_yield()`.
    
In portable, POSIX-compliant C code, `sched_yield()` is preferred when you want to be explicit about yielding without introducing an actual delay.
# Final Thoughts
This thread pool is simple by design. It avoids return values, uses POSIX threads, and gives you full control over memory and arguments. It’s a great base for lightweight concurrent work in C.

