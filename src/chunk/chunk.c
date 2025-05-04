#include "chunk.h"

static enum mcc_block_type mcc_chunk_get_block(mcc_world_seed_t, size_t x, size_t y, size_t z) {
    if (x == 8 && z == 8 && y >= 4 && y <= 9)
        return MCC_BLOCK_TYPE_LOG;

    if (x >= 6 && z >= 6 && x <= 10 && z <= 10 && y >= 8 && y <= 10)
        return MCC_BLOCK_TYPE_LEAVES;
 
    if (y == 0)
        return MCC_BLOCK_TYPE_STONE;
    else if (y == 1 || y == 2)
        return MCC_BLOCK_TYPE_DIRT;
    else if (y == 3)
        return MCC_BLOCK_TYPE_GRASS;
    else
        return MCC_BLOCK_TYPE_AIR;
}

void mcc_chunk_generate(mcc_world_seed_t seed, struct mcc_chunk_data *chunk) {
    for (size_t dx = 0; dx < MCC_CHUNK_WIDTH; dx++) {
        for (size_t dy = 0; dy < MCC_CHUNK_HEIGHT; dy++) {
            for (size_t dz = 0; dz < MCC_CHUNK_WIDTH; dz++) {
                size_t idx = mcc_chunk_block_idx(dx, dy, dz);

                size_t x = chunk->x * MCC_CHUNK_WIDTH + dx;
                size_t y = dy;
                size_t z = chunk->z * MCC_CHUNK_WIDTH + dz;
                chunk->blocks[idx] = mcc_chunk_get_block(seed, x, y, z);
            }
        }
    }
}
