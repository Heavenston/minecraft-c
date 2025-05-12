#define _GNU_SOURCE

#include "thread_pool.h"
#include "safe_cast.h"
#include "worksteal/worksteal.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <semaphore.h>
#include <sys/types.h>
#include <xcb/xproto.h>
#include <unistd.h>

struct wt_pub_data {
    pthread_t pthread_id;
};

struct wt_priv_data {
    struct mcc_thread_pool *pool;
    size_t thread_index;
};

struct mcc_thread_pool {
    /**
     * Queue of tasks for the whole pool, pushing or taking requires locking
     * to `queue_mutex`.
     */
    struct mcc_worksteal_queue queue;
    /**
     * Mutex for the /queue/.
     */
    pthread_mutex_t queue_mutex;
    /**
     * Condition for waiting the queue to fill up.
     */
    pthread_cond_t queue_cond;
    #ifndef NDEBUG
    /**
     * Used for checking the thread pool has correctly been locked before pushing tasks
     */
    pid_t locking_queue_thread_id;
    #endif
    size_t thread_count;
    struct wt_pub_data *threads;
};

_Atomic(struct mcc_thread_pool*) o_global_thread_pool = NULL;

/**
 * Function started on each thread that starts each dispatched tasks.
 */
static void *thread_routine(void *r_void_data) {
    assert(r_void_data != NULL);
    struct wt_priv_data *priv = r_void_data;
    struct mcc_thread_pool *pool = priv->pool;

    for (;;) {
        mcc_worksteal_data_type_t task;
        enum mcc_worksteal_steal_result result =
            mcc_worksteal_queue_steal(&pool->queue, &task);

        switch (result) {
        // No tasks available means taking the more expansive path of taking
        // exclusive access to the queue and waiting for a task to take.
        case MCC_WORKSTEAL_STEAL_RESULT_EMPTY:
            pthread_mutex_lock(&pool->queue_mutex);
            // We can just take because we have exclusive queue access
            while (mcc_worksteal_queue_take(&pool->queue, &task) == MCC_WORKSTEAL_TAKE_RESULT_EMPTY) {
                pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
            }
            pthread_mutex_unlock(&pool->queue_mutex);
        /* FALLTHROUGH */
        case MCC_WORKSTEAL_STEAL_RESULT_SUCCESS:
            task.function(task.data);
            continue;
        case MCC_WORKSTEAL_STEAL_RESULT_ABORT:
            continue;
        }
    }

    return NULL;
}

static struct mcc_thread_pool *create_thread_pool() {
    struct mcc_thread_pool *new_thread_pool = malloc(sizeof(*new_thread_pool));
    assert(new_thread_pool != NULL);

    int nprocs = get_nprocs();
    fprintf(stderr, "Detected %i cores!\n", nprocs);

    mcc_worksteal_queue_init(&new_thread_pool->queue, 16);
    pthread_mutex_init(&new_thread_pool->queue_mutex, NULL);
    pthread_cond_init(&new_thread_pool->queue_cond, NULL);
    #ifndef NDEBUG
    new_thread_pool->locking_queue_thread_id = -1;
    #endif

    new_thread_pool->thread_count = safe_to_size_t(nprocs);
    new_thread_pool->threads = calloc(new_thread_pool->thread_count, sizeof(*new_thread_pool->threads));

    for (size_t thread_i = 0; thread_i < new_thread_pool->thread_count; thread_i++) {
        struct wt_priv_data *priv = calloc(1, sizeof(*priv));

        priv->pool = new_thread_pool;
        priv->thread_index = thread_i;

        new_thread_pool->threads[thread_i] = (struct wt_pub_data){
            // Will be ovewritten by pthread_create
            .pthread_id = 0,
        };

        int create_result = pthread_create(
            &new_thread_pool->threads[thread_i].pthread_id,
            &(pthread_attr_t){},
            thread_routine,
            priv
        );

        (void)create_result;
        assert(create_result == 0);
    }

    return new_thread_pool;
}

/**
 * Creates a new thread pool and tries to store it in the global thread pool variable
 */
static struct mcc_thread_pool *init_global_thread_pool() {
    struct mcc_thread_pool *new_thread_pool = create_thread_pool();

    struct mcc_thread_pool *expected = NULL;
    if (atomic_compare_exchange_strong(&o_global_thread_pool, &expected, new_thread_pool))
        return new_thread_pool;
    struct mcc_thread_pool *pool = atomic_load_explicit(&o_global_thread_pool, memory_order_relaxed);
    assert(pool != NULL);
    return pool;
}

struct mcc_thread_pool *mcc_thread_pool_global() {
    struct mcc_thread_pool *pool = atomic_load_explicit(&o_global_thread_pool, memory_order_relaxed);
    if (!pool)
        return init_global_thread_pool();
    return pool;
}

void mcc_thread_pool_lock(struct mcc_thread_pool *pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    #ifndef NDEBUG
    pool->locking_queue_thread_id = gettid();
    #endif
}

void mcc_thread_pool_push_task(struct mcc_thread_pool *pool, struct mcc_thread_pool_task task) {
    #ifndef NDEBUG
    assert(pool->locking_queue_thread_id == gettid());
    #endif
    mcc_worksteal_queue_push(&pool->queue, (mcc_worksteal_data_type_t){ task.fn, task.data });
}

void mcc_thread_pool_unlock(struct mcc_thread_pool *pool) {
    #ifndef NDEBUG
    assert(pool->locking_queue_thread_id == gettid());
    pool->locking_queue_thread_id = -1;
    #endif
    // We only broadcast the cond after pushing all tasks to reduce
    // spurious wake ups from the worker threads
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
}
