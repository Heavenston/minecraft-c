#include "worksteal.h"
#include "utils.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void mcc_worksteal_queue_init(struct mcc_worksteal_queue *queue, size_t capacity) {
    atomic_init(&queue->bottom, 0);
    atomic_init(&queue->top, 0);

    struct mcc_worksteal_array *array = malloc(sizeof(*array) + sizeof(mcc_worksteal_data_type_t) * capacity);
    assert(array != NULL);
    atomic_init(&array->capacity, capacity);
    atomic_init(&queue->array, array);
}

void mcc_worksteal_queue_resize(struct mcc_worksteal_queue *queue, size_t target_capacity) {
    // FIXME: Does it need atomic_loads ?
    size_t b = atomic_load_explicit(&queue->bottom, memory_order_relaxed);
    size_t t = atomic_load_explicit(&queue->top, memory_order_relaxed);

    size_t new_capacity = mcc_next_pow_of_2(target_capacity);

    struct mcc_worksteal_array *old_data = atomic_load_explicit(&queue->array, memory_order_relaxed);
    struct mcc_worksteal_array *new_data = malloc(sizeof(*new_data) + sizeof(mcc_worksteal_data_type_t) * new_capacity);
    assert(new_data != NULL);
    new_data->capacity = new_capacity;
    for (size_t i = t; i < b; i++)
        new_data->data[i % new_capacity] = old_data->data[i % old_data->capacity];

    atomic_store_explicit(&queue->array, new_data, memory_order_relaxed);
    free(old_data);
}

enum mcc_worksteal_take_result mcc_worksteal_queue_take(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t *out) {
    size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed);
    // NOTE: Not in the paper but bottom=0 causes a bug (because of the overflow? I dont understand why its not addressed in the paper so maybe its my fault)
    if (b == 0)
        return MCC_WORKSTEAL_TAKE_RESULT_EMPTY;
    b -= 1;
    struct mcc_worksteal_array *a = atomic_load_explicit(&q->array, memory_order_relaxed);
    atomic_store_explicit(&q->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    size_t t = atomic_load_explicit(&q->top, memory_order_relaxed);

    enum mcc_worksteal_take_result result;
    if (t <= b) {
        /* Non-empty queue. */
        *out = atomic_load_explicit(&a->data[b % a->capacity], memory_order_relaxed);
        result = MCC_WORKSTEAL_TAKE_RESULT_SUCCESS;
        if (t == b) {
            /* Single last element in queue. */
            if (!atomic_compare_exchange_strong_explicit(&q->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
                /* Failed race. */
                result = MCC_WORKSTEAL_TAKE_RESULT_EMPTY;
            }
            atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
        }
    } else { /* Empty queue. */
        result = MCC_WORKSTEAL_TAKE_RESULT_EMPTY;
        atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
    }
    return result;
}

void mcc_worksteal_queue_push(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t x) {
    size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed);
    size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
    struct mcc_worksteal_array *a = atomic_load_explicit(&q->array, memory_order_relaxed);
    if (b - t > a->capacity - 1) { /* Full queue. */
        mcc_worksteal_queue_resize(q, a->capacity + 1);
        a = atomic_load_explicit(&q->array, memory_order_relaxed);
    }
    atomic_store_explicit(&a->data[b % a->capacity], x, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
}

enum mcc_worksteal_steal_result mcc_worksteal_queue_steal(struct mcc_worksteal_queue *q, mcc_worksteal_data_type_t *out) {
    size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    size_t b = atomic_load_explicit(&q->bottom, memory_order_acquire);

    enum mcc_worksteal_steal_result result = MCC_WORKSTEAL_STEAL_RESULT_EMPTY;
    if (t < b) {
        /* Non-empty queue. */
        struct mcc_worksteal_array *a = atomic_load_explicit(&q->array, memory_order_consume);
        *out = atomic_load_explicit(&a->data[t % a->capacity], memory_order_relaxed);
        result = MCC_WORKSTEAL_STEAL_RESULT_SUCCESS;
        if (!atomic_compare_exchange_strong_explicit(&q->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
            /* Failed race. */
            return MCC_WORKSTEAL_STEAL_RESULT_ABORT;
        }
    }
    return result;
}
