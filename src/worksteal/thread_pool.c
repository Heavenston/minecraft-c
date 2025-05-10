#define _GNU_SOURCE

#include "thread_pool.h"
#include "safe_cast.h"
#include "worksteal/worksteal.h"
#include "utils.h"

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
    pthread_t                         id       ;
    struct mcc_worksteal_queue        queue    ;
    sem_t                             semaphore;
};

struct wt_priv_data {
    struct mcc_thread_pool *pool        ;
    size_t                            thread_index;
};

struct mcc_thread_pool {
    /**
     * 'seed' queue, used for adding new tasks to the thread pool from a thread
     * that is not part of the thread pool.
     */
    struct mcc_worksteal_queue seed_queue;
    /**
     * Mutex for the /seed_queue/, only required to use for pushing
     */
    pthread_mutex_t seed_queue_mutex;
    #ifndef NDEBUG
    /**
     * Used for checking the thread pool has correctly been locked before pushing tasks
     */
    pid_t locking_seed_queue_thread_id;
    #endif
    size_t thread_count;
    struct wt_pub_data *threads;
};

_Atomic(struct mcc_thread_pool*) o_global_thread_pool = NULL;

/**
 * Randomly tries all threads on the thread pool to find one with a task to steal
 */
static enum mcc_worksteal_take_result random_steal(struct mcc_thread_pool *pool, mcc_worksteal_data_type_t *out) {
    // We shuffle a list of all thread indexes so that we check each thread
    // in a random order.
    static _Thread_local size_t thread_idxs[2048];
    assert(pool->thread_count+1 < 2048);
    for (size_t i = 0; i < pool->thread_count; i++)
        thread_idxs[i+1] = i;
    shuffle(pool->thread_count, thread_idxs+1);
    // ~0 means seed queue
    thread_idxs[0] = ~0ul;

    for (size_t i = 0; i < pool->thread_count+1;) {
        struct mcc_worksteal_queue *queue = thread_idxs[i] == ~0ul
            ? &pool->seed_queue
            : &pool->threads[thread_idxs[i]].queue;
        enum mcc_worksteal_steal_result result =
            mcc_worksteal_queue_steal(queue, out);

        switch (result) {
        case MCC_WORKSTEAL_STEAL_RESULT_SUCCESS:
            return MCC_WORKSTEAL_TAKE_RESULT_SUCCESS;
        case MCC_WORKSTEAL_STEAL_RESULT_EMPTY:
            // Try next thread
            i++;
            continue;
        case MCC_WORKSTEAL_STEAL_RESULT_ABORT:
            // Try again
            // FIXME: Is the the right strategy ?
            break;
        }
    }

    // We tried all threads anyway
    return MCC_WORKSTEAL_TAKE_RESULT_EMPTY;
}

/**
 * Function started on each thread that starts each dispatched tasks.
 */
static void *thread_routine(void *r_void_data) {
    assert(r_void_data != NULL);
    struct wt_priv_data *priv = r_void_data;
    struct wt_pub_data *pub = &priv->pool->threads[priv->thread_index];

    for (;;) {
        mcc_worksteal_data_type_t task;
        enum mcc_worksteal_take_result result =
            mcc_worksteal_queue_take(&pub->queue, &task);

        if (result == MCC_WORKSTEAL_TAKE_RESULT_EMPTY) {
            result = random_steal(priv->pool, &task);
        }

        if (result == MCC_WORKSTEAL_TAKE_RESULT_EMPTY) {
            sem_wait(&pub->semaphore);
            continue;
        }

        assert(result == MCC_WORKSTEAL_TAKE_RESULT_SUCCESS);

        task.function(task.data);
    }

    return NULL;
}

static struct mcc_thread_pool *create_thread_pool() {
    struct mcc_thread_pool *new_thread_pool = malloc(sizeof(*new_thread_pool));
    assert(new_thread_pool != NULL);

    int nprocs = get_nprocs();
    fprintf(stderr, "Detected %i cores!\n", nprocs);

    mcc_worksteal_queue_init(&new_thread_pool->seed_queue, 16);
    pthread_mutex_init(&new_thread_pool->seed_queue_mutex, NULL);
    new_thread_pool->locking_seed_queue_thread_id = -1;

    new_thread_pool->thread_count = safe_to_size_t(nprocs);
    new_thread_pool->threads = calloc(new_thread_pool->thread_count, sizeof(*new_thread_pool->threads));

    for (size_t thread_i = 0; thread_i < new_thread_pool->thread_count; thread_i++) {
        mcc_worksteal_queue_init(&new_thread_pool->threads[thread_i].queue, 16);
        sem_init(&new_thread_pool->threads[thread_i].semaphore, 0, 0);

        pthread_attr_t attr = {};
        struct wt_priv_data *data = calloc(1, sizeof(*data));

        data->pool = new_thread_pool;
        data->thread_index = thread_i;

        pthread_create(
            &new_thread_pool->threads[thread_i].id,
            &attr,
            thread_routine,
            data
        );
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

ssize_t get_self_thread_idx(struct mcc_thread_pool *pool) {
    auto self = pthread_self();
    for (size_t i = 0; i < pool->thread_count; i++) {
        if (pthread_equal(pool->threads[i].id, self))
            return safe_to_ssize_t(i);
    }
    return -1;
}

bool mcc_thread_pool_is_worker_thread(struct mcc_thread_pool *pool) {
    return get_self_thread_idx(pool) != -1;
}

void mcc_thread_pool_lock(struct mcc_thread_pool *pool) {
    if (mcc_thread_pool_is_worker_thread(pool))
        return;
    pthread_mutex_lock(&pool->seed_queue_mutex);
    #ifndef NDEBUG
    pool->locking_seed_queue_thread_id = gettid();
    #endif
}

void mcc_thread_pool_push_task(struct mcc_thread_pool *pool, struct mcc_thread_pool_task task) {
    ssize_t self_thread_idx = get_self_thread_idx(pool);
    // We push from the seed queue if we are not part of the thread pool
    if (self_thread_idx == -1) {
        #ifndef NDEBUG
        assert(pool->locking_seed_queue_thread_id == gettid());
        #endif
        mcc_worksteal_queue_push(&pool->seed_queue, (mcc_worksteal_data_type_t){ task.fn, task.data });
    }
    // but if we are part of it we can just push from our own queue
    else {
        mcc_worksteal_queue_push(&pool->threads[self_thread_idx].queue, (mcc_worksteal_data_type_t){ task.fn, task.data });
    }
}

void mcc_thread_pool_unlock(struct mcc_thread_pool *pool) {
    if (mcc_thread_pool_is_worker_thread(pool))
        return;
    #ifndef NDEBUG
    assert(pool->locking_seed_queue_thread_id == gettid());
    pool->locking_seed_queue_thread_id = -1;
    #endif
    pthread_mutex_unlock(&pool->seed_queue_mutex);

    for (size_t i = 0; i < pool->thread_count; i++) {
        sem_post(&pool->threads[i].semaphore);
    }
}
