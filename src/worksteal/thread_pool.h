#pragma once

struct mcc_thread_pool;

typedef void (*mcc_thread_pool_task_fn)(void*);

struct mcc_thread_pool_task {
    mcc_thread_pool_task_fn fn;
    void *data;
};

struct mcc_thread_pool *mcc_thread_pool_global();

bool mcc_thread_pool_is_worker_thread(struct mcc_thread_pool *pool);

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
