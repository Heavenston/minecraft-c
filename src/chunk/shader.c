#include "shader.h"
#include "chunk.h"
#include "cpu_rasterizer/cpu_rasterizer.h"
#include "triangulate.h"
#include "linalg/vector.h"
#include "safe_cast.h"

#include <stddef.h>
#include <stdlib.h>

/**
 * Number of varyings shared from the vertex to fragment shaders:
 * 1. texid + block_type information
 */
#define MCC_CHUNK_SHADER_VARYING_COUNT 1

void mcc_chunk_vertex_shader_fn(struct mcc_cpurast_vertex_shader_input *input) {
    struct mcc_chunk_render_object *render_object = input->o_in_data;
    struct mcc_chunk_mesh *mesh = render_object->mesh;

    // Get vertex data
    size_t vertex_idx = input->in_vertex_idx;
    mcc_vec3f position = mesh->positions[vertex_idx];
    uint8_t texid = mesh->texids[vertex_idx];

    // Transform vertex position
    mcc_vec4f pos_homogeneous = (mcc_vec4f){{ position.x, position.y, position.z, 1.0f }};
    input->out_position = mcc_mat4f_mul_vec4f(render_object->mvp, pos_homogeneous);

    // Pass block type to fragment shader
    input->r_out_varyings[0].vec4f = (mcc_vec4f){{ (float)texid + 0.5f, 0.0f, 0.0f, 0.0f }};
}

void mcc_chunk_fragment_shader_fn(struct mcc_cpurast_fragment_shader_input *input) {
    // Extract block type 
    uint8_t block_type = (uint8_t)input->r_in_varyings[0].vec4f.x;

    // Choose color based on block type
    switch (block_type) {
        case MCC_BLOCK_TYPE_STONE:
            input->out_color = (mcc_vec4f){{ 0.5f, 0.5f, 0.5f, 1.0f }}; // Gray
            break;
        case MCC_BLOCK_TYPE_DIRT:
            input->out_color = (mcc_vec4f){{ 0.6f, 0.4f, 0.2f, 1.0f }}; // Brown
            break;
        case MCC_BLOCK_TYPE_GRASS:
            input->out_color = (mcc_vec4f){{ 0.0f, 0.8f, 0.0f, 1.0f }}; // Green
            break;
        case MCC_BLOCK_TYPE_LOG:
            input->out_color = (mcc_vec4f){{ 0.4f, 0.3f, 0.2f, 1.0f }}; // Dark brown
            break;
        case MCC_BLOCK_TYPE_LEAVES:
            input->out_color = (mcc_vec4f){{ 0.0f, 0.6f, 0.0f, 1.0f }}; // Dark green
            break;
        default:
            input->out_color = (mcc_vec4f){{ 1.0f, 0.0f, 1.0f, 1.0f }}; // Magenta for unknown
            break;
    }
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
