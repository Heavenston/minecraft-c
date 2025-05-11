#include "wait_counter.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

void mcc_wait_counter_init(struct mcc_wait_counter *r_counter, size_t initial_count) {
    atomic_init(&r_counter->counter, initial_count);
    pthread_cond_init(&r_counter->cond, NULL);
    pthread_mutex_init(&r_counter->mutex, NULL);
}

void mcc_wait_counter_free(struct mcc_wait_counter *o_counter) {
    if (!o_counter)
        return;
    pthread_cond_destroy(&o_counter->cond);
    pthread_mutex_destroy(&o_counter->mutex);
}

bool mcc_wait_counter_decrement(struct mcc_wait_counter *r_counter, size_t amount) {
    size_t prev_value = atomic_fetch_sub_explicit(&r_counter->counter, 1, memory_order_relaxed);
    // Asserts that there was no underflow
    // Trying to prevent underflows by saturating at 0 would require
    // compare and swap (or does it?) which does not seems like a reasonable tradeoff
    assert(prev_value >= amount);
    if (prev_value > amount)
        // the value has not reached 0
        return false;
    pthread_mutex_lock(&r_counter->mutex);
    // FIXME: Does this have performance cost as opposed to _signal ?
    //        in which case we may want a way to differenciate.
    pthread_cond_broadcast(&r_counter->cond);
    pthread_mutex_unlock(&r_counter->mutex);
    return true;
}

void mcc_wait_counter_wait(struct mcc_wait_counter *r_counter) {
    pthread_mutex_lock(&r_counter->mutex);
    while (atomic_load_explicit(&r_counter->counter, memory_order_relaxed) != 0)
        pthread_cond_wait(&r_counter->cond, &r_counter->mutex);
    pthread_mutex_unlock(&r_counter->mutex);
}

