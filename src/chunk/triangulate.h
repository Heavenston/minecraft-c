#pragma once

#include <stddef.h>

#include "linalg/vector.h"
#include "chunk.h"

enum mcc_chunk_block_faces: uint8_t {
    BLOCK_FACE_NX = 1 << 0,
    BLOCK_FACE_PX = 1 << 1,
    BLOCK_FACE_NY = 1 << 2,
    BLOCK_FACE_PY = 1 << 3,
    BLOCK_FACE_NZ = 1 << 4,
    BLOCK_FACE_PZ = 1 << 5,
};

struct mcc_chunk_mesh {
    size_t vertex_count;
    size_t vertex_capacity;
    mcc_vec3f *positions;
    mcc_vec3f *normals;
    mcc_vec2f *texcoords;
    uint8_t *texids;
    enum mcc_chunk_block_faces *faces;
};

void mcc_chunk_mesh_init(struct mcc_chunk_mesh *r_mesh);
void mcc_chunk_mesh_free(struct mcc_chunk_mesh *r_mesh);

/**
 * The `r_mesh` param must be initialized with `mcc_chunk_mesh_init`.
 * And the `r_chunk_data` must be valid chunk data.
 */
void mcc_chunk_mesh_create(struct mcc_chunk_mesh *r_mesh, struct mcc_chunk_data *r_chunk_data);
