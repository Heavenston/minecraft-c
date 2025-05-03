#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

/**
 * MurmurHash-based one-at-a-time hash function for string keys
 * Provides good distribution and low collision rate
 */
static size_t mcc_hmap_murmur_oaat_32(const void *key, size_t h) {
    const char *str = key;
    for (; *str; ++str) {
        h ^= (size_t)*str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

/**
 * Jenkins one-at-a-time hash function for string keys
 * Simple and efficient hash function with good distribution
 */
static size_t mcc_hmap_jenkins_oaat(const void *key, UNUSED size_t seed) {
    const char *str = key;
    size_t hash = 0;
    for (; *str; ++str) {
        hash += (size_t)*str;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/**
 * Default primary hash function
 * Uses Jenkins one-at-a-time hash algorithm
 */
static inline size_t mcc_hmap_default_hash_function(const void *key, size_t seed) {
    return mcc_hmap_jenkins_oaat(key, seed);
}

/**
 * Default secondary hash function for double hashing
 * Uses MurmurHash-based algorithm for good secondary dispersion
 */
static inline size_t mcc_hmap_default_second_hash_function(const void *key, size_t hash) {
    return mcc_hmap_murmur_oaat_32(key, hash);
}

/**
 * Default comparison function for keys
 * Uses strcmp for null-terminated string keys
 */
static inline int mcc_hmap_default_compare_function(const void *key1, const void *key2) {
    return strcmp(key1, key2);
}

struct mcc_hmap *mcc_hmap_create_default(void) {
    return mcc_hmap_create(&(struct mcc_hmap_create_params){ .capacity = 100 });
}

struct mcc_hmap *mcc_hmap_create(struct mcc_hmap_create_params *params) {
    struct mcc_hmap *res = calloc(1, sizeof(*res));
    if (!res)
        return res;

    res->capacity = params->capacity ? params->capacity : 100;

    res->compare_func =
        params->compare_func ? params->compare_func : mcc_hmap_default_compare_function;

    res->hash_func =
        params->hash_func ? params->hash_func : mcc_hmap_default_hash_function;

    res->second_hash_func = params->second_hash_func
        ? params->second_hash_func
        : mcc_hmap_default_second_hash_function;

    res->free_nodes = res->capacity;

    res->data = calloc(res->capacity, sizeof(*(res->data)));
    if (!res->data) {
        free(res);
        return NULL;
    }

    return res;
}

/**
 * Expands the hash map when it gets too full (less than 25% of nodes are free)
 * Doubles the capacity and rehashes all elements
 * 
 * @param hmap The hash map to expand
 * @return true if expansion succeeded or wasn't needed, false if memory allocation failed
 */
static bool mcc_hmap_expand(struct mcc_hmap *hmap) {
    if (hmap->free_nodes <= hmap->capacity / 4) {
        struct mcc_hmap_data *new_data =
            calloc(hmap->capacity * 2, sizeof(*new_data));
        if (!new_data)
            return false;

        size_t new_cap = hmap->capacity * 2;
        for (size_t i = 0; i < hmap->capacity; i++) {
            struct mcc_hmap_data d = hmap->data[i];

            if (d.state == MCC_HMAP_OCCUPIED) {
                size_t h = d.hash_value;
                while (new_data[h % new_cap].state == MCC_HMAP_OCCUPIED)
                    h = hmap->second_hash_func(d.key, h);

                new_data[h % new_cap] = d;
            }
        }

        hmap->free_nodes = new_cap - (hmap->capacity - hmap->free_nodes);
        hmap->capacity = new_cap;
        free(hmap->data);
        hmap->data = new_data;
    }

    return true;
}

bool mcc_hmap_add(struct mcc_hmap *hmap, void *key, void *value) {
    if (!mcc_hmap_expand(hmap))
        return false;

    size_t hash = hmap->hash_func(key, 0);
    size_t h = hash;
    size_t cap = hmap->capacity;

    // Find a non-occupied slot or detect if key already exists
    while (hmap->data[h % cap].state == MCC_HMAP_OCCUPIED) {
        if (hmap->compare_func(key, hmap->data[h % cap].key) == 0)
            return false;

        h = hmap->second_hash_func(key, h);
    }

    // Double-check to make sure the key doesn't exist elsewhere in the chain
    for (size_t hh = h; hmap->data[hh % cap].state != MCC_HMAP_FREED;
         hh = hmap->second_hash_func(key, hh))
        if (hmap->data[hh % cap].state == MCC_HMAP_OCCUPIED
            && hmap->compare_func(key, hmap->data[hh % cap].key) == 0)
            return false;

    // Add the new key-value pair
    hmap->data[h % cap] = (struct mcc_hmap_data){
        .state = MCC_HMAP_OCCUPIED, 
        .key = key, 
        .hash_value = hash, 
        .value = value
    };
    hmap->free_nodes--;
    return true;
}

struct mcc_hmap_element mcc_hmap_remove(struct mcc_hmap *hmap, void *key) {
    size_t hash = hmap->hash_func(key, 0);
    for (struct mcc_hmap_data d = hmap->data[hash % hmap->capacity];
         d.state != MCC_HMAP_FREED; d = hmap->data[hash % hmap->capacity]) {
        if (d.state == MCC_HMAP_OCCUPIED && hmap->compare_func(key, d.key) == 0) {
            hmap->free_nodes++;
            hmap->data[hash % hmap->capacity].state = MCC_HMAP_DELETED;
            return (struct mcc_hmap_element){ .key = d.key, .value = d.value };
        }

        hash = hmap->second_hash_func(key, hash);
    }

    return (struct mcc_hmap_element){ .key = NULL, .value = NULL };
}

struct mcc_hmap_element mcc_hmap_find(struct mcc_hmap *hmap, void *key) {
    size_t hash = hmap->hash_func(key, 0);
    for (struct mcc_hmap_data d = hmap->data[hash % hmap->capacity];
         d.state != MCC_HMAP_FREED; d = hmap->data[hash % hmap->capacity]) {
        if (d.state == MCC_HMAP_OCCUPIED && hmap->compare_func(key, d.key) == 0)
            return (struct mcc_hmap_element){ .key = d.key, .value = d.value };

        hash = hmap->second_hash_func(key, hash);
    }

    return (struct mcc_hmap_element){ .key = NULL, .value = NULL };
}

void *mcc_hmap_update(struct mcc_hmap *hmap, void *key, void *value) {
    if (!mcc_hmap_expand(hmap))
        return MCC_ERROR_PTR;

    size_t hash = hmap->hash_func(key, 0);
    size_t h = hash;
    size_t cap = hmap->capacity;

    // Try to find and update an existing key
    while (hmap->data[h % cap].state == MCC_HMAP_OCCUPIED) {
        if (hmap->compare_func(key, hmap->data[h % cap].key) == 0) {
            void *res = hmap->data[h % cap].value;
            hmap->data[h % cap].value = value;
            return res;
        }

        h = hmap->second_hash_func(key, h);
    }

    // Double-check other locations in the chain
    for (size_t hh = h; hmap->data[hh % cap].state != MCC_HMAP_FREED;
         hh = hmap->second_hash_func(key, hh))
        if (hmap->data[hh % cap].state == MCC_HMAP_OCCUPIED
            && hmap->compare_func(key, hmap->data[hh % cap].key) == 0) {
            void *res = hmap->data[hh % cap].value;
            hmap->data[hh % cap].value = value;
            return res;
        }

    // Key not found, add it as a new entry
    hmap->data[h % cap] = (struct mcc_hmap_data){
        .state = MCC_HMAP_OCCUPIED, 
        .key = key, 
        .hash_value = hash, 
        .value = value
    };
    hmap->free_nodes--;
    return NULL;
}

void *mcc_hmap_update_with_func(struct mcc_hmap *hmap, void *key,
                               void *(*func)(void *previous_value)) {
    if (!mcc_hmap_expand(hmap))
        return MCC_ERROR_PTR;

    size_t hash = hmap->hash_func(key, 0);
    size_t h = hash;
    size_t cap = hmap->capacity;

    // Try to find and update an existing key using the provided function
    while (hmap->data[h % cap].state == MCC_HMAP_OCCUPIED) {
        if (hmap->compare_func(key, hmap->data[h % cap].key) == 0) {
            void *res = hmap->data[h % cap].value;
            hmap->data[h % cap].value = func(res);
            return res;
        }

        h = hmap->second_hash_func(key, h);
    }

    // Double-check other locations in the chain
    for (size_t hh = h; hmap->data[hh % cap].state != MCC_HMAP_FREED;
         hh = hmap->second_hash_func(key, hh))
        if (hmap->data[hh % cap].state == MCC_HMAP_OCCUPIED
            && hmap->compare_func(key, hmap->data[hh % cap].key) == 0) {
            void *res = hmap->data[hh % cap].value;
            hmap->data[hh % cap].value = func(res);
            return res;
        }

    // Key not found, add it with func(NULL) as the value
    hmap->data[h % cap] = (struct mcc_hmap_data){
        .state = MCC_HMAP_OCCUPIED, 
        .key = key, 
        .hash_value = hash, 
        .value = func(NULL)
    };
    hmap->free_nodes--;
    return NULL;
}

void mcc_hmap_destroy(struct mcc_hmap *hmap) {
    free(hmap->data);
    free(hmap);
}

void mcc_hmap_for_all(struct mcc_hmap *hmap,
                     void (*func)(void *key, void *value)) {
    for (size_t i = 0; i < hmap->capacity; i++) {
        if (hmap->data[i].state == MCC_HMAP_OCCUPIED)
            func(hmap->data[i].key, hmap->data[i].value);
    }
}
