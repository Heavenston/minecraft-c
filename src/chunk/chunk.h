#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define MCC_CHUNK_WIDTH 16

typedef uint64_t mcc_world_seed_t;

enum mcc_block_type: uint8_t {
    MCC_BLOCK_TYPE_AIR    = 0,
    MCC_BLOCK_TYPE_STONE  = 1,
    MCC_BLOCK_TYPE_DIRT   = 2,
    MCC_BLOCK_TYPE_GRASS  = 3,
    MCC_BLOCK_TYPE_LOG    = 4,
    MCC_BLOCK_TYPE_LEAVES = 5,
};

struct mcc_chunk_data {
    enum mcc_block_type blocks[MCC_CHUNK_WIDTH*MCC_CHUNK_WIDTH*MCC_CHUNK_WIDTH];
    /**
     * Posision of the chunk in chunk coordinates (global coordinates / MCC_CHUNK_WIDTH)
     */
    size_t x, y, z;
};

inline static bool mcc_block_is_transparent(enum mcc_block_type bt) {
    switch (bt) {
    case MCC_BLOCK_TYPE_AIR:
        return true;
    case MCC_BLOCK_TYPE_STONE:
    case MCC_BLOCK_TYPE_DIRT:
    case MCC_BLOCK_TYPE_GRASS:
    case MCC_BLOCK_TYPE_LOG:
    case MCC_BLOCK_TYPE_LEAVES:
        return false;
    }
    return false;
}

inline static size_t mcc_chunk_block_idx(size_t x, size_t y, size_t z) {
    assert(x < MCC_CHUNK_WIDTH);
    assert(y < MCC_CHUNK_WIDTH);
    assert(z < MCC_CHUNK_WIDTH);
    return x + z * MCC_CHUNK_WIDTH + y * MCC_CHUNK_WIDTH * MCC_CHUNK_WIDTH;
}

void mcc_chunk_generate(mcc_world_seed_t seed, struct mcc_chunk_data *chunk);
