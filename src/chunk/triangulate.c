#include "triangulate.h"
#include "chunk/chunk.h"
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void mcc_chunk_mesh_init(struct mcc_chunk_mesh *r_mesh) {
    r_mesh->vertex_count = 0;
    r_mesh->vertex_capacity = 0;
    r_mesh->positions = NULL;
    r_mesh->normals = NULL;
    r_mesh->texcoords = NULL;
    r_mesh->texids = NULL;
}

void mcc_chunk_mesh_free(struct mcc_chunk_mesh *r_mesh) {
    free(r_mesh->positions);
    free(r_mesh->normals);
    free(r_mesh->texcoords);
    free(r_mesh->texids);

    r_mesh->vertex_count = 0;
    r_mesh->vertex_capacity = 0;
    r_mesh->positions = NULL;
    r_mesh->normals = NULL;
    r_mesh->texcoords = NULL;
    r_mesh->texids = NULL;
}

/**
 * Increment the vertex_count and allocate required additional capacity.
 */
static void add_vertices(struct mcc_chunk_mesh *r_mesh, size_t amount) {
    size_t new_cap = r_mesh->vertex_capacity;
    if (new_cap == 0)
        new_cap = 8;
    while (new_cap < r_mesh->vertex_count + amount)
        new_cap *= 2;

    if (r_mesh->vertex_capacity < new_cap) {
        r_mesh->vertex_capacity = new_cap;
        r_mesh->normals = realloc(r_mesh->normals, sizeof(*r_mesh->normals) * new_cap);
        r_mesh->positions = realloc(r_mesh->positions, sizeof(*r_mesh->positions) * new_cap);
        r_mesh->texcoords = realloc(r_mesh->texcoords, sizeof(*r_mesh->texcoords) * new_cap);
        r_mesh->texids = realloc(r_mesh->texids, sizeof(*r_mesh->texids) * new_cap);
    }

    r_mesh->vertex_count += amount;
}

struct face_mesh {
    struct mcc_chunk_mesh *r_mesh;
    size_t start_idx;
    float32_t x, y, z;
    float32_t extent_x, extent_y, extent_z;
    bool clockwise; // Controls winding order of the face
};

/**
 * Swap vertices to change triangle winding order
 */
static inline void swap_winding(struct mcc_chunk_mesh *mesh, size_t start_idx) {
    // Swap vertices for the first triangle (0,1,2)
    mcc_vec3f temp_pos = mesh->positions[start_idx+1];
    mesh->positions[start_idx+1] = mesh->positions[start_idx+2];
    mesh->positions[start_idx+2] = temp_pos;
    
    mcc_vec2f temp_tex = mesh->texcoords[start_idx+1];
    mesh->texcoords[start_idx+1] = mesh->texcoords[start_idx+2];
    mesh->texcoords[start_idx+2] = temp_tex;
    
    // Swap texids for first triangle if they exist
    if (mesh->texids) {
        uint8_t temp_texid = mesh->texids[start_idx+1];
        mesh->texids[start_idx+1] = mesh->texids[start_idx+2];
        mesh->texids[start_idx+2] = temp_texid;
    }
    
    // Swap vertices for the second triangle (3,4,5)
    temp_pos = mesh->positions[start_idx+4];
    mesh->positions[start_idx+4] = mesh->positions[start_idx+3];
    mesh->positions[start_idx+3] = temp_pos;
    
    temp_tex = mesh->texcoords[start_idx+4];
    mesh->texcoords[start_idx+4] = mesh->texcoords[start_idx+3];
    mesh->texcoords[start_idx+3] = temp_tex;
    
    // Swap texids for second triangle if they exist
    if (mesh->texids) {
        uint8_t temp_texid = mesh->texids[start_idx+4];
        mesh->texids[start_idx+4] = mesh->texids[start_idx+3];
        mesh->texids[start_idx+3] = temp_texid;
    }
}

static inline void append_face_x(struct face_mesh fm) {
    auto mesh = fm.r_mesh;
    size_t si = fm.start_idx;

    float32_t x = fm.x, y = fm.y, z = fm.z;

    // Default is counter-clockwise winding order
    mesh->positions[si+0] = (mcc_vec3f){{ x, y+0.f, z+0.f }};
    mesh->texcoords[si+0] = (mcc_vec2f){{ 0.f, 0.f }};
    mesh->positions[si+1] = (mcc_vec3f){{ x, y+0.f, z+fm.extent_z }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x, y+fm.extent_y, z+0.f }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x, y+fm.extent_y, z+0.f }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x, y+0.f, z+fm.extent_z }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x, y+fm.extent_y, z+fm.extent_z }};
    mesh->texcoords[si+5] = (mcc_vec2f){{ 1.f, 1.f }};

    // If clockwise is requested, swap vertices to reverse winding order
    if (fm.clockwise) {
        swap_winding(mesh, si);
    }
}

static inline void append_face_z(struct face_mesh fm) {
    auto mesh = fm.r_mesh;
    size_t si = fm.start_idx;

    float32_t x = fm.x, y = fm.y, z = fm.z;

    // Default is counter-clockwise winding order
    mesh->positions[si+0] = (mcc_vec3f){{ x+0.f, y+0.f, z }};
    mesh->texcoords[si+0] = (mcc_vec2f){{ 0.f, 0.f }};
    mesh->positions[si+1] = (mcc_vec3f){{ x+fm.extent_x, y+0.f, z }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x+0.f, y+fm.extent_y, z }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x+0.f, y+fm.extent_y, z }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x+fm.extent_x, y+0.f, z }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x+fm.extent_x, y+fm.extent_y, z }};
    mesh->texcoords[si+5] = (mcc_vec2f){{ 1.f, 1.f }};

    // If clockwise is requested, swap vertices to reverse winding order
    if (!fm.clockwise) {
        swap_winding(mesh, si);
    }
}

static inline void append_face_y(struct face_mesh fm) {
    auto mesh = fm.r_mesh;
    size_t si = fm.start_idx;

    float32_t x = fm.x, y = fm.y, z = fm.z;

    // Default is counter-clockwise winding order
    mesh->positions[si+0] = (mcc_vec3f){{ x+0.f, y, z+0.f }};
    mesh->texcoords[si+0] = (mcc_vec2f){{ 0.f, 0.f }};
    mesh->positions[si+1] = (mcc_vec3f){{ x+fm.extent_x, y, z+0.f }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x+0.f, y, z+fm.extent_z }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x+0.f, y, z+fm.extent_z }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x+fm.extent_x, y, z+0.f }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x+fm.extent_x, y, z+fm.extent_z }};
    mesh->texcoords[si+5] = (mcc_vec2f){{ 1.f, 1.f }};
    
    // If clockwise is requested, swap vertices to reverse winding order
    if (fm.clockwise) {
        swap_winding(mesh, si);
    }
}

enum chunk_block_faces: uint8_t {
    BLOCK_FACE_NX = 1 << 0,
    BLOCK_FACE_PX = 1 << 1,
    BLOCK_FACE_NY = 1 << 2,
    BLOCK_FACE_PY = 1 << 3,
    BLOCK_FACE_NZ = 1 << 4,
    BLOCK_FACE_PZ = 1 << 5,
};

enum triangulation_axis: uint8_t {
    TRIANGULATION_AXIS_X = 0,
    TRIANGULATION_AXIS_Y = 1,
    TRIANGULATION_AXIS_Z = 2,
};

struct chunk_meshing_faces {
    /**
     * For each block, assigns a bit for each face for wether it is not
     * obstructed and so wether a mesh surface should be put.
     */
    uint8_t to_mesh_faces[MCC_CHUNK_WIDTH*MCC_CHUNK_WIDTH*MCC_CHUNK_HEIGHT];
};

struct block_face_data {
    /**
     * Unique 'id' of the block face.
     */
    uint8_t index;
    enum chunk_block_faces face;
    enum triangulation_axis axis;
    bool is_negative;
    int8_t dx;
    int8_t dy;
    int8_t dz;
};

const struct block_face_data block_faces[6] = {
    {
        .index = 0, .face = BLOCK_FACE_NX,
        .axis = TRIANGULATION_AXIS_X,
        .is_negative = true,
        .dx =-1, .dy = 0, .dz = 0,
    },
    {
        .index = 1, .face = BLOCK_FACE_PX,
        .axis = TRIANGULATION_AXIS_X,
        .is_negative = false,
        .dx =+1, .dy = 0, .dz = 0,
    },
    {
        .index = 0, .face = BLOCK_FACE_NY,
        .axis = TRIANGULATION_AXIS_Y,
        .is_negative = true,
        .dx = 0, .dy =-1, .dz = 0,
    },
    {
        .index = 1, .face = BLOCK_FACE_PY,
        .axis = TRIANGULATION_AXIS_Y,
        .is_negative = false,
        .dx = 0, .dy =+1, .dz = 0,
    },
    {
        .index = 0, .face = BLOCK_FACE_NZ,
        .axis = TRIANGULATION_AXIS_Z,
        .is_negative = true,
        .dx = 0, .dy = 0, .dz =-1,
    },
    {
        .index = 1, .face = BLOCK_FACE_PZ,
        .axis = TRIANGULATION_AXIS_Z,
        .is_negative = false,
        .dx = 0, .dy = 0, .dz =+1,
    },
};

size_t size_t_signed_add(size_t val, ssize_t diff) {
    return diff < 0 ? val - (size_t)(-diff) : val + (size_t)diff;
}

void mcc_chunk_mesh_create(struct mcc_chunk_mesh *r_mesh, struct mcc_chunk_data *r_chunk_data) {
    struct chunk_meshing_faces *meshing_faces = calloc(1, sizeof(*meshing_faces));

    for (size_t y = 0; y < MCC_CHUNK_HEIGHT; y++) {
        for (size_t z = 0; z < MCC_CHUNK_WIDTH; z++) {
            for (size_t x = 0; x < MCC_CHUNK_WIDTH; x++) {
                size_t block_idx = mcc_chunk_block_idx(x, y, z);
                enum mcc_block_type bt = r_chunk_data->blocks[block_idx];
                if (bt == MCC_BLOCK_TYPE_AIR)
                    continue;

                for (size_t fi = 0; fi < 6; fi++) {
                    auto face = &block_faces[fi];
                    ssize_t neighbor_x = (ssize_t)x + face->dx;
                    ssize_t neighbor_y = (ssize_t)y + face->dy;
                    ssize_t neighbor_z = (ssize_t)z + face->dz;
                    enum mcc_block_type neighbor_bt =
                        neighbor_x >= 0 && neighbor_x < MCC_CHUNK_WIDTH &&
                        neighbor_y >= 0 && neighbor_y < MCC_CHUNK_HEIGHT &&
                        neighbor_z >= 0 && neighbor_z < MCC_CHUNK_WIDTH
                        ? r_chunk_data->blocks[mcc_chunk_block_idx((size_t)neighbor_x, (size_t)neighbor_y, (size_t)neighbor_z)]
                        : MCC_BLOCK_TYPE_AIR;
                    if (mcc_block_is_transparent(neighbor_bt))
                        meshing_faces->to_mesh_faces[block_idx] |= face->face;
                }
            }
        }
    }

    for (size_t y = 0; y < MCC_CHUNK_HEIGHT; y++) {
        for (size_t z = 0; z < MCC_CHUNK_WIDTH; z++) {
            for (size_t x = 0; x < MCC_CHUNK_WIDTH; x++) {
                size_t block_idx = mcc_chunk_block_idx(x, y, z);
                enum mcc_block_type bt = r_chunk_data->blocks[block_idx];
                if (bt == MCC_BLOCK_TYPE_AIR)
                    continue;

                for (size_t fi = 0; fi < 6; fi++) {
                    auto face = &block_faces[fi];
                    
                    if (!(meshing_faces->to_mesh_faces[block_idx] & face->face)) {
                        continue;
                    }

                    size_t extent_x = 0;
                    size_t extended_idx;
                    // Try to extend in the x direction with the condition
                    // that the block type is the same
                    while (
                        face->axis != TRIANGULATION_AXIS_X &&
                        x + extent_x + 1 < MCC_CHUNK_WIDTH &&
                        meshing_faces->to_mesh_faces[(extended_idx = mcc_chunk_block_idx(x + extent_x + 1, y, z))] & face->face &&
                        r_chunk_data->blocks[extended_idx] == bt
                    ) {
                        // If we extend this face's mesh to it we do not need to mesh it
                        meshing_faces->to_mesh_faces[extended_idx] &= ~face->face;
                        extent_x++;
                    }
                    size_t extent_z = 0;
                    while (
                        face->axis != TRIANGULATION_AXIS_Z &&
                        z + extent_z + 1 < MCC_CHUNK_WIDTH
                    ) {
                        for (size_t dx = 0; dx <= extent_x; dx++) {
                            extended_idx = mcc_chunk_block_idx(x + dx, y, z + extent_z + 1);
                            if (!(
                                meshing_faces->to_mesh_faces[extended_idx] & face->face &&
                                r_chunk_data->blocks[extended_idx] == bt
                            )) {
                                goto stop_extending_z;
                            }
                        }
                        for (size_t dx = 0; dx <= extent_x; dx++) {
                            extended_idx = mcc_chunk_block_idx(x + dx, y, z + extent_z + 1);
                            meshing_faces->to_mesh_faces[extended_idx] &= ~face->face;
                        }
                        extent_z++;

                        continue;
                        stop_extending_z:
                        break;
                    }
                    size_t extent_y = 0;
                    while (
                        face->axis != TRIANGULATION_AXIS_Y &&
                        y + extent_y + 1 < MCC_CHUNK_HEIGHT
                    ) {
                        for (size_t dx = 0; dx <= extent_x; dx++) {
                            for (size_t dz = 0; dz <= extent_z; dz++) {
                                extended_idx = mcc_chunk_block_idx(x + dx, y + extent_y + 1, z + dz);
                                if (!(
                                    meshing_faces->to_mesh_faces[extended_idx] & face->face &&
                                    r_chunk_data->blocks[extended_idx] == bt
                                )) {
                                    goto stop_extending_y;
                                }
                            }
                        }
                        for (size_t dx = 0; dx <= extent_x; dx++) {
                            for (size_t dz = 0; dz <= extent_z; dz++) {
                                extended_idx = mcc_chunk_block_idx(x + dx, y + extent_y + 1, z + dz);
                                meshing_faces->to_mesh_faces[extended_idx] &= ~face->face;
                            }
                        }
                        extent_y++;

                        continue;
                        stop_extending_y:
                        break;
                    }

                    struct face_mesh face_mesh = {
                        .r_mesh = r_mesh,
                        .start_idx = r_mesh->vertex_count,
                        .x = (float)x + (face->dx == 0 ? 0.f : face->dx < 0 ? 0.f : 1.f),
                        .y = (float)y + (face->dy == 0 ? 0.f : face->dy < 0 ? 0.f : 1.f),
                        .z = (float)z + (face->dz == 0 ? 0.f : face->dz < 0 ? 0.f : 1.f),
                        .extent_x = (float)(extent_x + 1),
                        .extent_y = (float)(extent_y + 1),
                        .extent_z = (float)(extent_z + 1),
                        .clockwise = !face->is_negative,
                    };
                    add_vertices(r_mesh, 6);

                    for (size_t i = face_mesh.start_idx; i < face_mesh.start_idx+6; i++)
                        r_mesh->normals[i] = (mcc_vec3f){{ (float)face->dx, (float)face->dy, (float)face->dz }};
                    for (size_t i = face_mesh.start_idx; i < face_mesh.start_idx+6; i++)
                        r_mesh->texids[i] = r_chunk_data->blocks[block_idx];

                    switch (face->axis) {
                    case TRIANGULATION_AXIS_X:
                        append_face_x(face_mesh);
                        break;
                    case TRIANGULATION_AXIS_Y:
                        append_face_y(face_mesh);
                        break;
                    case TRIANGULATION_AXIS_Z:
                        append_face_z(face_mesh);
                        break;
                    }
                }
            }
        }
    }

    free(meshing_faces);
}
