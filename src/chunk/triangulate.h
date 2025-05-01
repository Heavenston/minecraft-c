#pragma once

#include <stddef.h>

#include "linalg/vector.h"
#include "chunk.h"

struct mcc_chunk_mesh {
    size_t vertex_count;
    size_t vertex_capacity;
    mcc_vec3f *positions;
    mcc_vec3f *normals;
    mcc_vec2f *texcoords;
    uint8_t *texids;
};

void mcc_chunk_mesh_init(struct mcc_chunk_mesh *r_mesh);
void mcc_chunk_mesh_free(struct mcc_chunk_mesh *r_mesh);

/**
 * The `r_mesh` param must be initialized with `mcc_chunk_mesh_init`.
 * And the `r_chunk_data` must be valid chunk data.
 */
void mcc_chunk_mesh_create(struct mcc_chunk_mesh *r_mesh, struct mcc_chunk_data *r_chunk_data);
