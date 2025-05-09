#pragma once

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

typedef void * mcc_worksteal_data_type_t;

// NOTE: Using an array with its capacity directly before means we can
//       switch data and capacity in a single atomic operation
//       (changing the pointer to this struct)
struct mcc_worksteal_array {
    /**
     * Capacity of the buffer
     */
    atomic_size_t capacity;
    /**
     * Flexible array member
     */
    _Atomic(mcc_worksteal_data_type_t) data[];
};

struct mcc_worksteal_queue {
    // NOTE: `top < bottom`,
    // so bottom and top are only valid indices if you do `bottom % size`
    atomic_size_t bottom;
    atomic_size_t top;
    _Atomic(struct mcc_worksteal_array*) array;
};

enum mcc_worksteal_result {
    MCC_WORKSTEAL_RESULT_SUCCESS = 0,
    MCC_WORKSTEAL_RESULT_EMPTY   = 1,
    MCC_WORKSTEAL_RESULT_ABORT   = 2,
};

void mcc_worksteal_queue_init(struct mcc_worksteal_queue *queue, size_t capacity);

/**
 * Can only be called by the Queue's owner thread
 */
void mcc_worksteal_queue_resize(struct mcc_worksteal_queue *queue, size_t target_capacity);

/**
 * Can only be called by the Queue's owner thread
 */
enum mcc_worksteal_result mcc_worksteal_queue_take(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t *out);

/**
 * Can only be called by the Queue's owner thread
 */
void mcc_worksteal_queue_push(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t x);

/**
 * Can be called by any thread
 */
enum mcc_worksteal_result mcc_worksteal_queue_steal(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t *out);
