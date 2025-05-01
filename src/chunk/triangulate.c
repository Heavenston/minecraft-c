#include "triangulate.h"
#include "chunk/chunk.h"
#include "defs.h"

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
    
    // Swap vertices for the second triangle (3,4,5)
    temp_pos = mesh->positions[start_idx+4];
    mesh->positions[start_idx+4] = mesh->positions[start_idx+3];
    mesh->positions[start_idx+3] = temp_pos;
    
    temp_tex = mesh->texcoords[start_idx+4];
    mesh->texcoords[start_idx+4] = mesh->texcoords[start_idx+3];
    mesh->texcoords[start_idx+3] = temp_tex;
}

static inline void append_face_x(struct face_mesh fm) {
    auto mesh = fm.r_mesh;
    size_t si = fm.start_idx;

    float32_t x = fm.x, y = fm.y, z = fm.z;

    // Default is counter-clockwise winding order
    mesh->positions[si+0] = (mcc_vec3f){{ x, y+0.f, z+0.f }};
    mesh->texcoords[si+0] = (mcc_vec2f){{ 0.f, 0.f }};
    mesh->positions[si+1] = (mcc_vec3f){{ x, y+0.f, z+1.f }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x, y+1.f, z+0.f }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x, y+1.f, z+0.f }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x, y+0.f, z+1.f }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x, y+1.f, z+1.f }};
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
    mesh->positions[si+1] = (mcc_vec3f){{ x+1.f, y+0.f, z }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x+0.f, y+1.f, z }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x+0.f, y+1.f, z }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x+1.f, y+0.f, z }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x+1.f, y+1.f, z }};
    mesh->texcoords[si+5] = (mcc_vec2f){{ 1.f, 1.f }};

    // If clockwise is requested, swap vertices to reverse winding order
    if (fm.clockwise) {
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
    mesh->positions[si+1] = (mcc_vec3f){{ x+1.f, y, z+0.f }};
    mesh->texcoords[si+1] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+2] = (mcc_vec3f){{ x+0.f, y, z+1.f }};
    mesh->texcoords[si+2] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+3] = (mcc_vec3f){{ x+0.f, y, z+1.f }};
    mesh->texcoords[si+3] = (mcc_vec2f){{ 0.f, 1.f }};
    mesh->positions[si+4] = (mcc_vec3f){{ x+1.f, y, z+0.f }};
    mesh->texcoords[si+4] = (mcc_vec2f){{ 1.f, 0.f }};
    mesh->positions[si+5] = (mcc_vec3f){{ x+1.f, y, z+1.f }};
    mesh->texcoords[si+5] = (mcc_vec2f){{ 1.f, 1.f }};
    
    // If clockwise is requested, swap vertices to reverse winding order
    if (fm.clockwise) {
        swap_winding(mesh, si);
    }
}

struct chunk_block_meshing {
    struct mcc_chunk_mesh *r_mesh;
    struct mcc_chunk_data *r_chunk_data;
    enum mcc_block_type block_type;
    size_t x, y, z;
};

/**
 * Generate the face for the negative x face of the given block
 */
static inline void generate_block_face_nx(struct chunk_block_meshing m) {
    // TODO: Have some way to get neigbor chunk to check its data
    enum mcc_block_type neighbor =
        m.x == 0
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x - 1, m.y, m.z)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ -1.f, 0.f, 0.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_x((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x, .y = (float)m.y, .z = (float)m.z,
        .clockwise = true,  // We want clockwise winding for -X face (looking from outside)
    });
}

static inline void generate_block_face_px(struct chunk_block_meshing m) {
    // TODO: Have some way to get neigbor chunk to check its data
    enum mcc_block_type neighbor =
        m.x+1 >= MCC_CHUNK_WIDTH
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x + 1, m.y, m.z)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ 1.f, 0.f, 0.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_x((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x + 1.f, .y = (float)m.y, .z = (float)m.z,
        .clockwise = false,  // Counter-clockwise winding for +X face (looking from outside)
    });
}

static inline void generate_block_face_nz(struct chunk_block_meshing m) {
    // TODO: Have some way to get neighbor chunk to check its data
    enum mcc_block_type neighbor =
        m.z == 0
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x, m.y, m.z - 1)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ 0.f, 0.f, -1.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_z((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x, .y = (float)m.y, .z = (float)m.z,
        .clockwise = true,  // Clockwise winding for -Z face (looking from outside)
    });
}

static inline void generate_block_face_pz(struct chunk_block_meshing m) {
    // TODO: Have some way to get neighbor chunk to check its data
    enum mcc_block_type neighbor =
        m.z+1 >= MCC_CHUNK_WIDTH
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x, m.y, m.z + 1)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ 0.f, 0.f, 1.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_z((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x, .y = (float)m.y, .z = (float)m.z + 1.f,
        .clockwise = false,  // Counter-clockwise winding for +Z face (looking from outside)
    });
}

static inline void generate_block_face_ny(struct chunk_block_meshing m) {
    // TODO: Have some way to get neighbor chunk to check its data
    enum mcc_block_type neighbor =
        m.y == 0
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x, m.y - 1, m.z)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ 0.f, -1.f, 0.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_y((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x, .y = (float)m.y, .z = (float)m.z,
        .clockwise = true,  // Clockwise winding for -Y face (looking from outside)
    });
}

static inline void generate_block_face_py(struct chunk_block_meshing m) {
    // TODO: Have some way to get neighbor chunk to check its data
    enum mcc_block_type neighbor =
        m.y+1 >= MCC_CHUNK_HEIGHT
            ? MCC_BLOCK_TYPE_AIR
            : m.r_chunk_data->blocks[mcc_chunk_block_idx(m.x, m.y + 1, m.z)];

    if (!mcc_block_is_transparent(neighbor))
        return;

    size_t vi = m.r_mesh->vertex_count;
    add_vertices(m.r_mesh, 6);
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->normals[i] = (mcc_vec3f){{ 0.f, 1.f, 0.f }};
    for (size_t i = vi; i < vi+6; i++)
        m.r_mesh->texids[i] = m.block_type;

    append_face_y((struct face_mesh) {
        .r_mesh = m.r_mesh,
        .start_idx = vi,
        .x = (float)m.x, .y = (float)m.y + 1.f, .z = (float)m.z,
        .clockwise = false,  // Counter-clockwise winding for +Y face (looking from outside)
    });
}

void mcc_chunk_mesh_create(struct mcc_chunk_mesh *r_mesh, struct mcc_chunk_data *r_chunk_data) {
    for (size_t y = 0; y < MCC_CHUNK_HEIGHT; y++) {
        for (size_t z = 0; z < MCC_CHUNK_WIDTH; z++) {
            for (size_t x = 0; x < MCC_CHUNK_WIDTH; x++) {
                enum mcc_block_type bt = r_chunk_data->blocks[mcc_chunk_block_idx(x, y, z)];
                if (bt == MCC_BLOCK_TYPE_AIR)
                    continue;

                struct chunk_block_meshing m = {
                    .r_chunk_data = r_chunk_data,
                    .r_mesh = r_mesh,
                    .block_type = bt,
                    .x = x,
                    .y = y,
                    .z = z,
                };

                generate_block_face_nx(m);
                generate_block_face_px(m);
                generate_block_face_nz(m);
                generate_block_face_pz(m);
                generate_block_face_ny(m);
                generate_block_face_py(m);
            }
        }
    }
}
