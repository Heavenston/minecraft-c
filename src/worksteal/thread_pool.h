#pragma once

/**
 * A thread pool where tasks can be dispatched from multiple threads (with locking).
 * NOTE: *not* worksteal (designed for a single thread to dispatch task).
 * FIXME: Make additional per-thread queues, using workstealing for taking tasks
 *        making pushing new tasks a lockless operation ?
 */
struct mcc_thread_pool;

typedef void (*mcc_thread_pool_task_fn)(void*);

struct mcc_thread_pool_task {
    mcc_thread_pool_task_fn fn;
    void *data;
};

struct mcc_thread_pool *mcc_thread_pool_global();

/**
 * Locks the given pool to allow the current thread to use _push_task.
 * Does nothing if is a worker thread of this pool.
 */
void mcc_thread_pool_lock(struct mcc_thread_pool *pool);

/**
 * Adds a new task for the worker threads in the pool to use
 * the thread pool must be locked if accessed from outside of a worker thread.
 */
void mcc_thread_pool_push_task(struct mcc_thread_pool *pool, struct mcc_thread_pool_task task);

/**
 * Unlocks the given pool to allow the other threads to use _push_task.
 * Does nothing if is a worker thread of this pool.
 */
void mcc_thread_pool_unlock(struct mcc_thread_pool *pool);
