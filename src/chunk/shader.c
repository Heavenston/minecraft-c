#include "shader.h"
#include "chunk.h"
#include "cpu_rasterizer/cpu_rasterizer.h"
#include "triangulate.h"
#include "linalg/vector.h"
#include "linalg/scalars.h"
#include "images/ppm.h"
#include "safe_cast.h"

#include <stddef.h>
#include <math.h>
#include <stdlib.h>

/**
 * Number of varyings shared from the vertex to fragment shaders.
 */
#define MCC_CHUNK_SHADER_VARYING_COUNT 4

const uint8_t dirt_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/dirt.ppm"
#pragma GCC diagnostic pop
};
const uint8_t stone_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/stone.ppm"
#pragma GCC diagnostic pop
};
const uint8_t grass_block_side_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/grass_block_side.ppm"
#pragma GCC diagnostic pop
};
const uint8_t grass_block_top_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/grass_block_top.ppm"
#pragma GCC diagnostic pop
};
const uint8_t oak_leaves_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/oak_leaves.ppm"
#pragma GCC diagnostic pop
};
const uint8_t oak_log_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/oak_log.ppm"
#pragma GCC diagnostic pop
};
const uint8_t oak_log_top_data[] =
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#embed "../assets/oak_log_top.ppm"
#pragma GCC diagnostic pop
};

struct mcc_chunk_render_data {
    mcc_image_t dirt_texture;
    mcc_image_t stone_texture;
    mcc_image_t grass_block_side_texture;
    mcc_image_t grass_block_top_texture;
    mcc_image_t oak_leaves_texture;
    mcc_image_t oak_log_texture;
    mcc_image_t oak_log_top_texture;
};

struct mcc_chunk_render_data *mcc_chunk_render_data_load() {
    struct mcc_chunk_render_data *data = malloc(sizeof(*data));
    data->dirt_texture = mcc_ppm_load(dirt_data, sizeof(dirt_data));
    data->stone_texture = mcc_ppm_load(stone_data, sizeof(stone_data));
    data->grass_block_side_texture = mcc_ppm_load(grass_block_side_data, sizeof(grass_block_side_data));
    data->grass_block_top_texture = mcc_ppm_load(grass_block_top_data, sizeof(grass_block_top_data));
    data->oak_leaves_texture = mcc_ppm_load(oak_leaves_data, sizeof(oak_leaves_data));
    data->oak_log_texture = mcc_ppm_load(oak_log_data, sizeof(oak_log_data));
    data->oak_log_top_texture = mcc_ppm_load(oak_log_top_data, sizeof(oak_log_top_data));
    return data;
}

mcc_vec4f sample(mcc_image_t *image, mcc_vec2f texcoords) {
    size_t x = (size_t)((float)image->width * fmodf(texcoords.x, 1.f));
    size_t y = (size_t)((float)image->height * (1.f - fmodf(texcoords.y, 1.f)));
    size_t idx = x + y * image->width;
    uint8_t b = image->data[idx*4+0];
    uint8_t g = image->data[idx*4+1];
    uint8_t r = image->data[idx*4+2];
    uint8_t a = image->data[idx*4+3];

    return (mcc_vec4f){{ (float)r / 255.f, (float)g / 255.f, (float)b / 255.f, (float)a / 255.f }};
}

void mcc_chunk_render_data_free(struct mcc_chunk_render_data *data) {
    free(data);
}

void mcc_chunk_vertex_shader_fn(struct mcc_cpurast_vertex_shader_input *input) {
    struct mcc_chunk_render_object *render_object = input->o_in_data;
    struct mcc_chunk_mesh *mesh = render_object->mesh;

    // Get vertex data
    size_t vertex_idx = input->in_vertex_idx;
    mcc_vec3f position = mesh->positions[vertex_idx];
    mcc_vec3f normal = mesh->normals[vertex_idx];
    mcc_vec2f texcoords = mesh->texcoords[vertex_idx];
    uint8_t texid = mesh->texids[vertex_idx];
    enum mcc_chunk_block_faces face = mesh->faces[vertex_idx];

    // Transform vertex position
    mcc_vec4f pos_homogeneous = (mcc_vec4f){{ position.x, position.y, position.z, 1.0f }};
    input->out_position = mcc_mat4f_mul_vec4f(render_object->mvp, pos_homogeneous);

    // Pass block type to fragment shader
    input->r_out_varyings[0].vec4f = (mcc_vec4f){{
        (float)texid + 0.5f /* +0.5f is for the rounding to work predictably */,
        0.0f, 0.0f, 0.0f
    }};
    input->r_out_varyings[1].vec4f = (mcc_vec4f){{
        normal.x, normal.y, normal.z,
        0.f
    }};
    input->r_out_varyings[2].vec4f = (mcc_vec4f){{
        texcoords.u, texcoords.v,
        0.f, 0.f
    }};
    input->r_out_varyings[3].vec4f = (mcc_vec4f){{
        (float)face + 0.5f,
        0.f, 0.f, 0.f
    }};
}

void mcc_chunk_fragment_shader_fn(struct mcc_cpurast_fragment_shader_input *input) {
    struct mcc_chunk_render_object *render_object = input->o_in_data;
    struct mcc_chunk_render_data *render_data = render_object->data;

    uint8_t block_type = (uint8_t)input->r_in_varyings[0].vec4f.x;
    mcc_vec3f normal = input->r_in_varyings[1].vec4f.xyz;
    mcc_vec2f texcoords = input->r_in_varyings[2].vec4f.xy;
    enum mcc_chunk_block_faces face = input->r_in_varyings[3].vec4f.x;

    switch (block_type) {
        case MCC_BLOCK_TYPE_STONE:
            input->out_color = sample(&render_data->stone_texture, texcoords);
            break;
        case MCC_BLOCK_TYPE_DIRT:
            input->out_color = sample(&render_data->dirt_texture, texcoords);
            break;
        case MCC_BLOCK_TYPE_GRASS: {
            mcc_image_t *texture;
            switch (face) {
            case BLOCK_FACE_NX:
            case BLOCK_FACE_PX:
            case BLOCK_FACE_NZ:
            case BLOCK_FACE_PZ:
                texture = &render_data->grass_block_side_texture;
                break;
            case BLOCK_FACE_NY:
                texture = &render_data->dirt_texture;
                break;
            case BLOCK_FACE_PY:
                texture = &render_data->grass_block_top_texture;
                break;
            }
            input->out_color = sample(texture, texcoords);
            break;
        }
        case MCC_BLOCK_TYPE_LOG: {
            mcc_image_t *texture;
            switch (face) {
            case BLOCK_FACE_NX:
            case BLOCK_FACE_PX:
            case BLOCK_FACE_NZ:
            case BLOCK_FACE_PZ:
                texture = &render_data->oak_log_texture;
                break;
            case BLOCK_FACE_NY:
            case BLOCK_FACE_PY:
                texture = &render_data->oak_log_top_texture;
                break;
            }
            input->out_color = sample(texture, texcoords);
            break;
        }
        case MCC_BLOCK_TYPE_LEAVES:
            input->out_color = sample(&render_data->oak_leaves_texture, texcoords);
            break;
        default:
            input->out_color = (mcc_vec4f){{ 1.0f, 0.0f, 1.0f, 1.0f }}; // Magenta for unknown
            break;
    }

    mcc_vec3f strong_light_dir = mcc_vec3f_normalized((mcc_vec3f){{ 1.f, 2.5f, -1.5f }});
    mcc_vec3f soft_light_dir = mcc_vec3f_scale(strong_light_dir, -1.f);

    float strong_diffuse = clampf(mcc_vec3f_dot(normal, strong_light_dir), 0.f, 1.f);
    float soft_diffuse = clampf(mcc_vec3f_dot(normal, soft_light_dir), 0.f, 1.f);
    float ambient = 0.25f;
    float cf = clampf(strong_diffuse + soft_diffuse * 0.15f + ambient, 0.f, 1.f);

    input->out_color = mcc_vec4f_scale(input->out_color, cf);
    input->out_color.a = 1.f;
}

void mcc_chunk_vertex_shader(struct mcc_vertex_shader *out_shader) {
    out_shader->r_fn = mcc_chunk_vertex_shader_fn;
    out_shader->varying_count = MCC_CHUNK_SHADER_VARYING_COUNT;
}

void mcc_chunk_fragment_shader(struct mcc_fragment_shader *out_shader) {
    out_shader->r_fn = mcc_chunk_fragment_shader_fn;
    out_shader->varying_count = MCC_CHUNK_SHADER_VARYING_COUNT;
}

void mcc_chunk_render_config(
    struct mcc_cpurast_render_config *out_config,
    struct mcc_chunk_render_object *render_object,
    struct mcc_cpurast_rendering_attachment *attachment
) {
    out_config->r_attachment = attachment;
    
    out_config->o_vertex_shader_data = render_object;
    struct mcc_vertex_shader *vs = malloc(sizeof(struct mcc_vertex_shader));
    mcc_chunk_vertex_shader(vs);
    out_config->r_vertex_shader = vs;
    
    out_config->o_fragment_shader_data = render_object;
    struct mcc_fragment_shader *fs = malloc(sizeof(struct mcc_fragment_shader));
    mcc_chunk_fragment_shader(fs);
    out_config->r_fragment_shader = fs;
    
    out_config->culling_mode = MCC_CPURAST_CULLING_MODE_CCW;
    out_config->o_depth_comparison_fn = mcc_depth_comparison_fn_lt;
    out_config->polygon_mode = MCC_CPURAST_POLYGON_MODE_FILL;
    
    out_config->vertex_count = safe_to_u32(render_object->mesh->vertex_count);
    out_config->vertex_processing = MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST;
}

void mcc_chunk_render_config_cleanup(struct mcc_cpurast_render_config *config) {
    if (config->r_vertex_shader) {
        free((void*)config->r_vertex_shader);
    }
    if (config->r_fragment_shader) {
        free((void*)config->r_fragment_shader);
    }
}
