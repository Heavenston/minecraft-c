#pragma once

#include "chunk/triangulate.h"
#include "linalg/matrix.h"
#include "cpu_rasterizer/cpu_rasterizer.h"

struct mcc_chunk_render_object {
    struct mcc_chunk_mesh *mesh;
    mcc_mat4f mvp;
};

void mcc_chunk_vertex_shader_fn(struct mcc_cpurast_vertex_shader_input *input);
void mcc_chunk_fragment_shader_fn(struct mcc_cpurast_fragment_shader_input *input);

void mcc_chunk_vertex_shader(struct mcc_vertex_shader *out_shader);
void mcc_chunk_fragment_shader(struct mcc_fragment_shader *out_shader);

void mcc_chunk_render_config(
    struct mcc_cpurast_render_config *out_config,
    struct mcc_chunk_render_object *render_object,
    struct mcc_cpurast_rendering_attachment *attachment
);

void mcc_chunk_render_config_cleanup(struct mcc_cpurast_render_config *config);
