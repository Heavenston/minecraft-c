#pragma once

#include <pthread.h>
#include <stdatomic.h>

/**
 * Used for doing a fork-and-join, where a master threads wait for this counter
 * to reach zero, while worker threads decrement the counter for each finished
 * task.
 */
struct mcc_wait_counter {
    atomic_size_t counter;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

void mcc_wait_counter_init(struct mcc_wait_counter *r_counter, size_t initial_count);
void mcc_wait_counter_free(struct mcc_wait_counter *o_counter);

/**
 * Returns true if the counter reached 0.
 * Asserts that there is no underflow.
 */
bool mcc_wait_counter_decrement(struct mcc_wait_counter *r_counter, size_t amount);

/**
 * Blocks until the counter reaches 0.
 */
void mcc_wait_counter_wait(struct mcc_wait_counter *r_counter);
