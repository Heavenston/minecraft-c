#pragma once

#include "defs.h"
#include "color/color.h"
#include "linalg/vector.h"

#include <stddef.h>
#include <stdint.h>

struct mcc_cpurast_rendering_color_attachment {
    /**
     * Only one format is supported: BGRA
     */
    uint8_t *r_data;
};

struct mcc_cpurast_rendering_depth_attachment {
    /**
     * Only one format is supported: float32
     */
    float32_t *r_data;
};

struct mcc_cpurast_rendering_attachment {
    const struct mcc_cpurast_rendering_depth_attachment *o_depth;
    const struct mcc_cpurast_rendering_color_attachment *o_color;
    uint32_t width;
    uint32_t height;
};

union mcc_cpurast_shaders_varying {
    /**
     * For now this is the only possible type for varying parameters
     */
    mcc_vec4f vec4f;
};

struct mcc_cpurast_vertex_shader_input {
    void *o_in_data;

    uint32_t in_vertex_idx;
    /**
     * Pre alocated array of varyings of the length specified in the vertex struct
     */
    union mcc_cpurast_shaders_varying *r_out_varyings;

    mcc_vec4f out_position;
};

typedef void (*mcc_vertex_shader_fn)(struct mcc_cpurast_vertex_shader_input*);

struct mcc_vertex_shader {
    mcc_vertex_shader_fn r_fn;
    /**
     * Number of varying parameters this shader will output.
     */
    uint32_t varying_count;
};

struct mcc_cpurast_fragment_shader_input {
    void *o_in_data;

    /**
     * Length is as defined in the fragment shader struct
     */
    union mcc_cpurast_shaders_varying *r_in_varyings;

    mcc_vec3f in_frag_coord;

    mcc_vec4f out_color;
};

typedef void (*mcc_fragment_shader_fn)(struct mcc_cpurast_fragment_shader_input*);

struct mcc_fragment_shader {
    mcc_fragment_shader_fn r_fn;
    /**
     * Number of varying parameters this shader will output.
     */
    uint32_t varying_count;
};

struct mcc_cpurast_clear_config {
    const struct mcc_cpurast_rendering_attachment *r_attachment;
    /**
     * Ignored if `r_attachment->o_depth` is NULL
     */
    float32_t clear_depth;
    /**
     * Ignored if `r_attachment->o_color` is NULL
     */
    struct mcc_color_rgba clear_color;
};

void mcc_cpurast_clear(const struct mcc_cpurast_clear_config *r_config);

enum mcc_cpurast_culling_mode {
    MCC_CPURAST_CULLING_MODE_NONE,
    MCC_CPURAST_CULLING_MODE_CW,
    MCC_CPURAST_CULLING_MODE_CCW,
};

enum mcc_cpurast_vertex_processing {
    /**
     * Each set of 3 vertices represent the 3 corner of each triangle.
     */
    MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST,
    /**
     * 'The second and third vertex of every triangle are used as first two vertices of the next triangle'
     */
    MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP,
};

typedef bool (*mcc_depth_comparison_fn)(float previous, float new);

bool mcc_depth_comparison_fn_alaways(float previous, float new);
bool mcc_depth_comparison_fn_never(float previous, float new);
bool mcc_depth_comparison_fn_gt(float previous, float new);
bool mcc_depth_comparison_fn_gte(float previous, float new);
bool mcc_depth_comparison_fn_lt(float previous, float new);
bool mcc_depth_comparison_fn_lte(float previous, float new);
bool mcc_depth_comparison_fn_eq(float previous, float new);
bool mcc_depth_comparison_fn_neq(float previous, float new);

struct mcc_cpurast_render_config {
    struct mcc_cpurast_rendering_attachment *r_attachment;

    void *o_fragment_shader_data;
    struct mcc_fragment_shader *r_fragment_shader;
    void *o_vertex_shader_data;
    struct mcc_vertex_shader *r_vertex_shader;

    enum mcc_cpurast_culling_mode culling_mode;
    /**
     * Ignored if r_attachment->o_depth is NULL.
     * 
     * If set to NULL, no fragment will be discarded regardless of depth,
     * but depth will still be written to the depth attachment.
     *
     * If not null, will be used to compare depths of new fragments and
     * what is stored in the depth buffer, discarding the fragment
     * without incoving the fragment shader if the function returns
     * false.
     */
    mcc_depth_comparison_fn o_depth_comparison_fn;

    /**
     * Number of vertices to render. For now there is no vertex buffers, use internal ones!
     */
    uint32_t vertex_count;
    /**
     * How to process each vertices.
     */
    enum mcc_cpurast_vertex_processing vertex_processing;
};

void mcc_cpurast_render(const struct mcc_cpurast_render_config *r_config);
