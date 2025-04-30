#pragma once

/**
 * Hash Map adapted from my 42sh's implementation.
 * Thanks to the 69Team :)
 */

#include <stdbool.h>
#include <stddef.h>

#ifndef MCC_ERROR_PTR
#define MCC_ERROR_PTR (void *)~0ul
#endif

/**
 * Node state for open addressing implementation
 */
enum mcc_hmap_node_state {
    MCC_HMAP_FREED,    /* Empty slot, never been used */
    MCC_HMAP_OCCUPIED, /* Currently holds a key-value pair */
    MCC_HMAP_DELETED,  /* Previously occupied but now deleted */
};

/**
 * Generic hash map structure that uses open addressing with double hashing
 * for collision resolution
 */
struct mcc_hmap {
    /* Array of hash map data nodes */
    struct mcc_hmap_data {
        /* Pointer to the key */
        void *key;
        
        /* Pointer to the value */
        void *value;
        
        /* Cached hash value for faster rehashing */
        size_t hash_value;
        
        /* Current state of this node (freed, occupied, or deleted) */
        enum mcc_hmap_node_state state;
    } *data;
    
    /* Total allocated capacity */
    size_t capacity;
    
    /* Number of remaining free nodes (resizes when < 25% free) */
    size_t free_nodes;
    
    /* Primary hash function */
    size_t (*hash_func)(const void *key, size_t seed);
    
    /* Secondary hash function for collision resolution */
    size_t (*second_hash_func)(const void *key, size_t hash);
    
    /* Key comparison function */
    int (*compare_func)(const void *key1, const void *key2);
};

/**
 * Parameters for initializing a hash map
 */
struct mcc_hmap_create_params {
    /* Initial capacity of the hash map */
    size_t capacity;
    
    /* Primary hash function */
    size_t (*hash_func)(const void *, size_t);
    
    /* Secondary hash function for collision resolution */
    size_t (*second_hash_func)(const void *, size_t);
    
    /* Key comparison function */
    int (*compare_func)(const void *, const void *);
};

/**
 * Key-value pair stored in the hash map
 */
struct mcc_hmap_element {
    /* Pointer to the key */
    void *key;
    
    /* Pointer to the value */
    void *value;
};

/**
 * Creates a new hash map with default parameters
 * Default capacity is 100, default hash functions work with null-terminated string keys
 * 
 * @return Pointer to the newly created hash map or NULL if allocation failed
 */
struct mcc_hmap *mcc_hmap_create_default(void);

/**
 * Creates a new hash map with the provided parameters
 * 
 * @param params Configuration parameters for the hash map
 * @return Pointer to the newly created hash map or NULL if allocation failed
 */
struct mcc_hmap *mcc_hmap_create(struct mcc_hmap_create_params *params);

/**
 * Adds a key-value pair to the hash map
 * 
 * @param hmap The hash map
 * @param key The key to add
 * @param value The value to associate with the key
 * @return true if addition was successful, false if the key already exists or memory allocation failed
 */
bool mcc_hmap_add(struct mcc_hmap *hmap, void *key, void *value);

/**
 * Removes an element identified by the key from the hash map
 * 
 * @param hmap The hash map
 * @param key The key to remove
 * @return The removed element (key-value pair), or {NULL, NULL} if not found
 */
struct mcc_hmap_element mcc_hmap_remove(struct mcc_hmap *hmap, void *key);

/**
 * Finds an element identified by the key in the hash map
 * 
 * @param hmap The hash map
 * @param key The key to find
 * @return The found element (key-value pair), or {NULL, NULL} if not found
 */
struct mcc_hmap_element mcc_hmap_find(struct mcc_hmap *hmap, void *key);

/**
 * Updates the value of an element identified by the key in the hash map
 * If the element doesn't exist, it will be added
 * 
 * @param hmap The hash map
 * @param key The key to update
 * @param value The new value to associate with the key
 * @return The old value if updated, NULL if added, or MCC_ERROR_PTR if an error occurred
 */
void *mcc_hmap_update(struct mcc_hmap *hmap, void *key, void *value);

/**
 * Updates the value of an element using a function that transforms the old value
 * If the element doesn't exist, it will be added with func(NULL) as the value
 * 
 * @param hmap The hash map
 * @param key The key to update
 * @param func Function that takes the old value and returns the new value
 * @return The old value if updated, NULL if added, or MCC_ERROR_PTR if an error occurred
 */
void *mcc_hmap_update_with_func(struct mcc_hmap *hmap, void *key,
                               void *(*func)(void *previous_value));

/**
 * Destroys the hash map and frees all associated memory
 * Does not free the memory for keys and values
 * 
 * @param hmap The hash map to destroy
 */
void mcc_hmap_destroy(struct mcc_hmap *hmap);

/**
 * Applies a function to all key-value pairs in the hash map
 * Useful for freeing each element or performing other operations on all elements
 * 
 * @param hmap The hash map
 * @param func Function to apply to each key-value pair
 */
void mcc_hmap_for_all(struct mcc_hmap *hmap,
                     void (*func)(void *key, void *value));
