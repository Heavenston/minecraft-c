#include "cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/vector.h"
#include "linalg/scalars.h"
#include "safe_cast.h"
#include "utils.h"
#include "worksteal/thread_pool.h"
#include "worksteal/wait_counter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct processed_vertex {
    union mcc_cpurast_shaders_varying *varyings;
    mcc_vec4f pos_homogeneous;
    float w_inv;
};

struct plane {
    mcc_vec4f normal;
    float D;
};

typedef union {
    struct {
        struct processed_vertex v0;
        struct processed_vertex v1;
        struct processed_vertex v2;
    };
    struct processed_vertex vertices[3];
} primitive_t;

const struct plane clipping_planes[] = {
    { .normal = (mcc_vec4f){{ +0.f, +0.f, +1.f, +1.f }}, .D = +0.00f },
    { .normal = (mcc_vec4f){{ +0.f, +0.f, -1.f, +1.f }}, .D = +0.00f },

    // NOTE: Clipping -x +x -y +y seems useless as said by lisyarus's blog
    //       but does it have any advantage ??
    // { .normal = (mcc_vec4f){{ +0.f, +1.f, +0.f, +1.f }}, .D = +0.00f },
    // { .normal = (mcc_vec4f){{ +0.f, -1.f, +0.f, +1.f }}, .D = +0.00f },

    // { .normal = (mcc_vec4f){{ +1.f, +0.f, +0.f, +1.f }}, .D = +0.00f },
    // { .normal = (mcc_vec4f){{ -1.f, +0.f, +0.f, +1.f }}, .D = +0.00f },
};

#define NUMBER_OF_CLIPPING_PLANES (sizeof(clipping_planes) / sizeof(*clipping_planes))
/**
 * Maximum number of triangles after clipping a single triangle
 * Defined as the 2^(the number of clipping planes)
 * (because each clipping plane may didive each triangle in two)
 */
#define MAX_SUBTRIANGLES (1UL << NUMBER_OF_CLIPPING_PLANES)
/**
 * Number of varyings to allocate at the start of rasterization.
 */
#define PREALLOCATED_VARYINGS_SIZE (MAX_SUBTRIANGLES * 6)

typedef struct {
    union mcc_cpurast_shaders_varying *v[PREALLOCATED_VARYINGS_SIZE];
    /**
     * Index of the next free (currently unused) varying array.
     */
    size_t free_head;
} preallocated_varyings_t;

static float signed_distance(struct plane plane, mcc_vec4f vertex) {
    return mcc_vec4f_dot(plane.normal, vertex) + plane.D;
}

/**
 * Context for calculating sub triangles form a single triangle intersecting
 * a list of hardcoded clip planes
 */
struct mcc_triangle_clip_context {
    const size_t varying_count;
    /**
     * Assumes to have enough memory for MAX_SUBTRIANGLES triangles
     * and to have a single initial triangle (3 vertices)
     */
    mcc_vec4f * const r_primitive_buffer;
    /**
     * Number of vertices outputed.
     */
    size_t out_vertex_count;
};

/**
 * Context for calculating sub triangles form a single triangle intersecting
 * a single plane.
 * The convoluted workings of appending/overwrite of the primitives is intended
 * to remove the need for a temporary buffer (by means of premature optimisation (TM))
 */
struct plane_clip_triangle_context {
    const size_t varying_count;
    /**
     * Assumes to have enough memory for any required triangles.
     */
    mcc_vec4f * const r_primitive_buffer;
    /**
     * Number of primitives in the primitive buffer, may be higher than 1
     * in which case the other primitives will be left untouched, and
     * if there is a new primitive created it will be apended after them extending
     * the buffer size.
     */
    const size_t primitive_buffer_count;
    /**
     * Initially one, can be set to 0, 1 or 2 depending of the clipping results.
     * If set to 0, this means the first primitive of the buffer should be
     * removed.
     * If set to 1 this means the first primitive of the buffer has been overwritten 
     * with the new primitive.
     * If the so 2, the previous primitive has been overwritten as well as a new
     * primitive was appended to the end of the buffer (after primitive_buffer_count)
     */
    size_t in_out_triangle_count;
    const struct plane plane;
};

typedef struct intersection_context {
    struct plane_clip_triangle_context *ctctx;
    mcc_vec4f *v0;
    mcc_vec4f *v1;
    float value0;
    float value1;
    /**
     * Out vertex, may be the same as v0 or v1
     */
    mcc_vec4f *out;
} ic_t;

static void clip_edge(ic_t ctx) {
    float t = ctx.value0 / (ctx.value0 - ctx.value1);

    for (uint32_t i = 0; i < ctx.ctctx->varying_count+1; i++) {
        ctx.out[i] = mcc_vec4f_add(
            mcc_vec4f_scale(ctx.v0[i], 1.f - t),
            mcc_vec4f_scale(ctx.v1[i], t)
        );
    }
}

typedef struct copy_edge_context {
    struct plane_clip_triangle_context *ctctx;
    mcc_vec4f *in;
    mcc_vec4f *out;
} cc_t;

static void copy_edge(cc_t ctx) {
    for (uint32_t i = 0; i < ctx.ctctx->varying_count+1; i++) {
        ctx.out[i] = ctx.in[i];
    }
}

/*
 * Adapted from
 * https://www.gabrielgambetta.com/computer-graphics-from-scratch/11-clipping.html
 * And
 * https://lisyarus.github.io/blog/posts/implementing-a-tiny-cpu-rasterizer-part-5.html
 */
static void clip_triangle_once(struct plane_clip_triangle_context *ctx) {
    struct plane plane = ctx->plane;

    // This makes indexing easier as to not require the multiplication by `ctx->varying_count + 1` each time
    mcc_vec4f (*buf)[ctx->varying_count + 1] = (void*)ctx->r_primitive_buffer;
    size_t in_v0 = 0 * 3 + 0;
    size_t in_v1 = 0 * 3 + 1;
    size_t in_v2 = 0 * 3 + 2;
    size_t ou_v0 = ctx->primitive_buffer_count * 3 + 0;
    size_t ou_v1 = ctx->primitive_buffer_count * 3 + 1;
    size_t ou_v2 = ctx->primitive_buffer_count * 3 + 2;

    float values[3] = {
        signed_distance(plane, buf[in_v0][0]),
        signed_distance(plane, buf[in_v1][0]),
        signed_distance(plane, buf[in_v2][0])
    };

    uint8_t mask = (values[0] <= 0.f ? 1 : 0) | (values[1] <= 0.f ? 2 : 0) | (values[2] <= 0.f ? 4 : 0);

    switch (mask) {
    case 0b000:
        // No changes
        ctx->in_out_triangle_count = 1;
        return;
    case 0b001: {
        clip_edge((ic_t){ ctx, buf[in_v0], buf[in_v1], values[0], values[1], buf[ou_v0] });
        // output.triangles[output.triangle_count].v0 = v01;
        copy_edge((cc_t){ ctx, buf[in_v1],                                   buf[ou_v1] });
        // output.triangles[output.triangle_count].v1 = triangle.v1;
        copy_edge((cc_t){ ctx, buf[in_v2],                                   buf[ou_v2] });
        // output.triangles[output.triangle_count].v2 = triangle.v2;

        clip_edge((ic_t){ ctx, buf[in_v0], buf[in_v2], values[0], values[2], buf[in_v2] });
        // output.triangles[output.triangle_count].v2 = v02;
        clip_edge((ic_t){ ctx, buf[in_v0], buf[in_v1], values[0], values[1], buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = v01;
        copy_edge((cc_t){ ctx, buf[ou_v2 /* in place of in_v2 */],           buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = triangle.v2;

        ctx->in_out_triangle_count = 2;
        break;
    }
    case 0b010: {
        copy_edge((cc_t){ ctx, buf[in_v0],                                   buf[ou_v0] });
        // output.triangles[output.triangle_count].v0 = triangle.v0;
        clip_edge((ic_t){ ctx, buf[in_v1], buf[in_v0], values[1], values[0], buf[ou_v1] });
        // output.triangles[output.triangle_count].v1 = v10;
        copy_edge((cc_t){ ctx, buf[in_v2],                                   buf[ou_v2] });
        // output.triangles[output.triangle_count].v2 = triangle.v2;

        clip_edge((ic_t){ ctx, buf[in_v1], buf[in_v2], values[1], values[2], buf[in_v2] });
        // output.triangles[output.triangle_count].v2 = v12;
        clip_edge((ic_t){ ctx, buf[in_v1], buf[in_v0], values[1], values[0], buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = v10;
        copy_edge((cc_t){ ctx, buf[ou_v2],                                   buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = triangle.v2;

        ctx->in_out_triangle_count = 2;
        break;
    }
    case 0b011: {
        clip_edge((ic_t){ ctx, buf[in_v0], buf[in_v2], values[0], values[2], buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = v02;
        clip_edge((ic_t){ ctx, buf[in_v1], buf[in_v2], values[1], values[2], buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = v12;
        copy_edge((cc_t){ ctx, buf[in_v2],                                   buf[in_v2] });
        // output.triangles[output.triangle_count].v2 = triangle.v2;

        ctx->in_out_triangle_count = 1;
        break;
    }
    case 0b100: {
        copy_edge((cc_t){ ctx, buf[in_v0],                                   buf[ou_v0] });
        // output.triangles[output.triangle_count].v0 = triangle.v0;
        copy_edge((cc_t){ ctx, buf[in_v1],                                   buf[ou_v1] });
        // output.triangles[output.triangle_count].v1 = triangle.v1;
        clip_edge((ic_t){ ctx, buf[in_v2], buf[in_v0], values[2], values[0], buf[ou_v2] });
        // output.triangles[output.triangle_count].v2 = v20;

        clip_edge((ic_t){ ctx, buf[in_v2], buf[in_v0], values[2], values[0], buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = v20;
        copy_edge((cc_t){ ctx, buf[in_v1],                                   buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = triangle.v1;
        clip_edge((ic_t){ ctx, buf[in_v2], buf[ou_v1], values[2], values[1], buf[in_v2] });
        //                                      ^ in place of in_v1
        // output.triangles[output.triangle_count].v2 = v21;

        ctx->in_out_triangle_count = 2;
        break;
    }
    case 0b101: {
        clip_edge((ic_t){ ctx, buf[in_v0], buf[in_v1], values[0], values[1], buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = v01;
        copy_edge((cc_t){ ctx, buf[in_v1],                                   buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = triangle.v1;
        clip_edge((ic_t){ ctx, buf[in_v2], buf[in_v1], values[2], values[1], buf[in_v2] });
        // output.triangles[output.triangle_count].v2 = v21;

        ctx->in_out_triangle_count = 1;
        break;
    }
    case 0b110: {
        copy_edge((cc_t){ ctx, buf[in_v0],                                   buf[in_v0] });
        // output.triangles[output.triangle_count].v0 = triangle.v0;
        clip_edge((ic_t){ ctx, buf[in_v1], buf[in_v0], values[1], values[0], buf[in_v1] });
        // output.triangles[output.triangle_count].v1 = v10;
        clip_edge((ic_t){ ctx, buf[in_v2], buf[in_v0], values[2], values[0], buf[in_v2] });
        // output.triangles[output.triangle_count].v2 = v20;

        ctx->in_out_triangle_count = 1;
        break;
    }
    case 0b111:
        ctx->in_out_triangle_count = 0;
        break;
    }
}
 
void mcc_cpurast_clear(const struct mcc_cpurast_clear_config *r_config) {
    uint32_t size = r_config->r_attachment->height * r_config->r_attachment->width;

    auto color_attachment = r_config->r_attachment->o_color;
    if (color_attachment) {
        for (uint32_t i = 0; i < size * 4; i += 4) {
            color_attachment->r_data[i+0] = (uint8_t)(r_config->clear_color.b * 255.f);
            color_attachment->r_data[i+1] = (uint8_t)(r_config->clear_color.g * 255.f);
            color_attachment->r_data[i+2] = (uint8_t)(r_config->clear_color.r * 255.f);
            color_attachment->r_data[i+3] = (uint8_t)(r_config->clear_color.a * 255.f);
        }
    }

    auto depth_attachment = r_config->r_attachment->o_depth;
    if (depth_attachment) {
        for (uint32_t i = 0; i < size; i++) {
            depth_attachment->r_data[i] = r_config->clear_depth;
        }
    }
}

bool mcc_depth_comparison_fn_alaways(float, float) {
    return true;
}

bool mcc_depth_comparison_fn_never(float, float) {
    return false;
}

bool mcc_depth_comparison_fn_gt(float previous, float new) {
    return new > previous;
}

bool mcc_depth_comparison_fn_gte(float previous, float new) {
    return new >= previous;
}

bool mcc_depth_comparison_fn_lt(float previous, float new) {
    return new < previous;
}

bool mcc_depth_comparison_fn_lte(float previous, float new) {
    return new <= previous;
}

bool mcc_depth_comparison_fn_eq(float previous, float new) {
    return new == previous;
}

bool mcc_depth_comparison_fn_neq(float previous, float new) {
    return new != previous;
}

struct mcc_barycentric_coords {
    /**
     * Weight for v1
     */
    float u;
    /**
     * Weight for v2
     */
    float v;
    /**
     * Weight for v0
     */
    float w;
};

struct mcc_pixel_params {
    struct mcc_barycentric_coords barycentric;
    mcc_vec2f screen_pos;
    float depth;
    size_t pixel_idx;
};

typedef bool (*mcc_polygon_filter_fn)(const struct mcc_barycentric_coords *coords);

struct mcc_rasterization_context {
    enum mcc_cpurast_culling_mode culling_mode;
    struct mcc_cpurast_rendering_attachment *r_attachment;
    primitive_t *r_primitive;
    union mcc_cpurast_shaders_varying *r_fragment_varyings;
    struct mcc_cpurast_fragment_shader_input *r_frag_input;
    struct mcc_fragment_shader *r_fragment_shader;
    mcc_depth_comparison_fn o_depth_comparison_fn;
    size_t varying_count;
};

typedef void (*mcc_rasterize_triangle_fn)(const struct mcc_rasterization_context*);

static void process_fragment(const struct mcc_rasterization_context *r_context, const struct mcc_pixel_params *pixel) {
    auto depth_attachment = r_context->r_attachment->o_depth;
    auto color_attachment = r_context->r_attachment->o_color;
    primitive_t *primitive = r_context->r_primitive;
    const struct mcc_barycentric_coords *barycentric = &pixel->barycentric;
    
    // Interpolation of varying attributes
    for (size_t varying_i = 0; varying_i < r_context->varying_count; varying_i++) {
        float w0             = primitive->v0.w_inv,
              w1             = primitive->v1.w_inv,
              w2             = primitive->v2.w_inv,
              w_interpolated = barycentric->w * w0 + barycentric->u * w1 + barycentric->v * w2,
              correction     = 1.0f / w_interpolated;
        // TEMP: Disables perspective correction
        // correction = 1.f;
        
        mcc_vec4f var0 = primitive->v0.varyings[varying_i].vec4f,
                  var1 = primitive->v1.varyings[varying_i].vec4f,
                  var2 = primitive->v2.varyings[varying_i].vec4f;

        mcc_vec4f result = mcc_vec4f_add(
            mcc_vec4f_add(
                mcc_vec4f_scale(var0, barycentric->w * w0 * correction),
                mcc_vec4f_scale(var1, barycentric->u * w1 * correction)
            ),
            mcc_vec4f_scale(var2, barycentric->v * w2 * correction)
        );
        
        r_context->r_fragment_varyings[varying_i].vec4f = result;
    }

    // Execute the fragment shader
    r_context->r_frag_input->in_frag_coord = (mcc_vec3f){{ pixel->screen_pos.x, pixel->screen_pos.y, pixel->depth }};
    r_context->r_fragment_shader->r_fn(r_context->r_frag_input);

    // Write depth if we have a depth attachment
    if (depth_attachment) {
        depth_attachment->r_data[pixel->pixel_idx] = pixel->depth;
    }

    // Write color if we have a color attachment
    if (color_attachment) {
        color_attachment->r_data[pixel->pixel_idx*4+0] = (uint8_t)(r_context->r_frag_input->out_color.b * 255.f);
        color_attachment->r_data[pixel->pixel_idx*4+1] = (uint8_t)(r_context->r_frag_input->out_color.g * 255.f);
        color_attachment->r_data[pixel->pixel_idx*4+2] = (uint8_t)(r_context->r_frag_input->out_color.r * 255.f);
        color_attachment->r_data[pixel->pixel_idx*4+3] = (uint8_t)(r_context->r_frag_input->out_color.a * 255.f);
    }
}

static void rasterize_triangle(const struct mcc_rasterization_context *r_context) {
    auto depth_attachment = r_context->r_attachment->o_depth;
    primitive_t *primitive = r_context->r_primitive;

    // Get vertex positions
    mcc_vec3f v0 = mcc_vec3f_scale(primitive->v0.pos_homogeneous.xyz, primitive->v0.w_inv),
              v1 = mcc_vec3f_scale(primitive->v1.pos_homogeneous.xyz, primitive->v1.w_inv),
              v2 = mcc_vec3f_scale(primitive->v2.pos_homogeneous.xyz, primitive->v2.w_inv);
    mcc_vec2f p0 = v0.xy,
              p1 = v1.xy,
              p2 = v2.xy;

    mcc_vec2f p1p0 = mcc_vec2f_sub(p1, p0);
    mcc_vec2f p2p1 = mcc_vec2f_sub(p2, p1);
    mcc_vec2f p0p2 = mcc_vec2f_sub(p0, p2);
    mcc_vec2f p2p0 = mcc_vec2f_sub(p2, p0);
    float det012 = mcc_mat2f_det(mcc_mat2f_col(p1p0, p2p0));

    bool isCcw = det012 < 0.f;

    if ((r_context->culling_mode == MCC_CPURAST_CULLING_MODE_CW && isCcw) ||
        (r_context->culling_mode == MCC_CPURAST_CULLING_MODE_CCW && !isCcw))
        return;

    // TODO
    // if (r_context->culling_mode != MCC_CPURAST_CULLING_MODE_CCW && isCcw) {
    //     mcc_vec2f t2 = p1;
    //     p1 = p2;
    //     p2 = t2;
    //     mcc_vec3f t3 = v1;
    //     v1 = v2;
    //     v2 = t3;
    //     struct processed_vertex tv = primitive->v1;
    //     primitive->v1 = primitive->v2;
    //     primitive->v2 = tv;
    // }

    // Screen dimensions as floats
    float wf = (float)r_context->r_attachment->width;
    float hf = (float)r_context->r_attachment->height;

    // AABB of the triangle in viewport coordinate
    float min_xf = clampf(min3f(p0.x, p1.x, p2.x), -1.f, 1.f);
    float min_yf = clampf(min3f(p0.y, p1.y, p2.y), -1.f, 1.f);
    float max_xf = clampf(max3f(p0.x, p1.x, p2.x), -1.f, 1.f);
    float max_yf = clampf(max3f(p0.y, p1.y, p2.y), -1.f, 1.f);

    // AABB of the triangle in screen coordinate
    // NOTE: Since y is inverted we need to take the min pos to have the max pixel
    uint32_t min_x = (uint32_t)(( min_xf + 1.f) * 0.5f * wf - 0.5f);
    uint32_t min_y = (uint32_t)((-max_yf + 1.f) * 0.5f * hf - 0.5f);
    uint32_t max_x = (uint32_t)(( max_xf + 1.f) * 0.5f * wf + 0.5f);
    uint32_t max_y = (uint32_t)((-min_yf + 1.f) * 0.5f * hf + 0.5f);

    assert(max_x <= r_context->r_attachment->width);
    assert(max_y <= r_context->r_attachment->height);
    assert(min_x <= max_x);
    assert(min_y <= max_y);

    // Iterate over pixels
    for (uint32_t y = min_y; y < max_y; y++) {
        for (uint32_t x = min_x; x < max_x; x++) {
            size_t pixel_idx = x + y * r_context->r_attachment->width;

            const float fx = (float)x + 0.5f,
                        fy = (float)y + 0.5f;

            // Normalized screen coordinates
            const mcc_vec2f screen_pos = {{
                fx * 2.f / wf - 1.f,
                fy *-2.f / hf + 1.f,
            }};

            // Calculate barycentric coordinates
            const float det01p = mcc_mat2f_det(mcc_mat2f_col(p1p0, mcc_vec2f_sub(screen_pos, p0)));
            const float det12p = mcc_mat2f_det(mcc_mat2f_col(p2p1, mcc_vec2f_sub(screen_pos, p1)));
            const float det20p = mcc_mat2f_det(mcc_mat2f_col(p0p2, mcc_vec2f_sub(screen_pos, p2)));

            const bool isInside = det01p >= 0.f && det12p >= 0.f && det20p >= 0.f;
            if (!isInside)
                continue;
            const struct mcc_barycentric_coords barycentric = {
                .u = det20p / det012,
                .v = det01p / det012,
                .w = det12p / det012,
            };

            // Calculate depth using barycentric coordinates
            float depth = v1.z * barycentric.u + v2.z * barycentric.v + v0.z * barycentric.w;

            // Depth comparison function test
            if (
                depth_attachment &&
                r_context->o_depth_comparison_fn &&
                !r_context->o_depth_comparison_fn(depth_attachment->r_data[pixel_idx], depth)
            ) {
                continue;
            }

            // Process this fragment
            struct mcc_pixel_params pixel = {
                .barycentric = barycentric,
                .screen_pos = screen_pos,
                .depth = depth,
                .pixel_idx = pixel_idx
            };
            process_fragment(r_context, &pixel);
        }
    }
}

static void clip_triangle(struct mcc_triangle_clip_context *ctx) {
    const size_t tri_buff_size = (ctx->varying_count + 1) * 3;
    const size_t tri_byte_size = tri_buff_size * sizeof(mcc_vec4f);

    size_t triangle_count = 1;

    for (size_t plane_i = 0; plane_i < sizeof(clipping_planes) / sizeof(*clipping_planes); plane_i++) {
        // We process the whole primitive buffer each time, but we also
        // modify it inline, modifying the current primitive, as well as adding
        // new ones at the end.
        // Removing the current primitive is done by shifting the whole buffer
        // one to the left (cannot do a swap remove by copying the last one
        // because if new ones were added before the last one may not be one we
        // should clip right now)
        
        size_t previous_triangle_count = triangle_count;
        for (size_t triangle_i = 0; triangle_i < previous_triangle_count;) {
            struct plane_clip_triangle_context sctx = {
                .varying_count = ctx->varying_count,
                .r_primitive_buffer = ctx->r_primitive_buffer + triangle_i * tri_buff_size,
                .primitive_buffer_count = triangle_count,
                .in_out_triangle_count = 1,
                .plane = clipping_planes[plane_i],
            };
            clip_triangle_once(&sctx);

            switch (sctx.in_out_triangle_count) {
            case 0:
                // Shifts the whole buffer one primitive to the left
                
                if (triangle_count > triangle_i + 1) {
                    memmove(
                        &sctx.r_primitive_buffer[tri_buff_size * triangle_i],
                        &sctx.r_primitive_buffer[tri_buff_size * (triangle_i + 1)],
                        tri_byte_size * (triangle_count - triangle_i - 1)
                    );
                }

                // One less primitive now
                assert(triangle_count > 0);
                triangle_count -= 1;
                
                // We do not increment primitive_i, the buffer moved, not the
                // index
                previous_triangle_count--;
                break;
            case 1:
                // Noting special to do here (the primitive was just edited inline)
                triangle_i++;
                break;
            case 2:
                // One primitive was added to the end, only difference.
                triangle_count += 1;
                triangle_i++;
                break;
            }
        }
    }

    ctx->out_vertex_count = triangle_count * 3;
}

struct vertex_process_task_data {
    void *o_fragment_shader_data;
    struct mcc_fragment_shader *r_fragment_shader;
    void *o_vertex_shader_data;
    struct mcc_vertex_shader *r_vertex_shader;

    enum mcc_cpurast_culling_mode culling_mode;
    enum mcc_cpurast_vertex_processing vertex_processing;

    uint32_t vertex_start_idx;
    uint32_t vertex_count;

    /**
     * To be decremented when the task is finished (if != NULL)
     */
    struct mcc_wait_counter *o_wait_counter;

    /**
     * Buffer where to store all proccessed and clipped triangles
     * must be of at least `vertex_count * MAX_SUBTRIANGLES * (varying_count + 1)` size
     */
    mcc_vec4f *r_out_primitive_buffer;
    uint32_t out_vertex_count;
};

static void vertex_process_task(void *r_void_data) {
    struct vertex_process_task_data *r_data = r_void_data;
    assert(r_data != NULL);

    assert(r_data->r_fragment_shader->varying_count == r_data->r_vertex_shader->varying_count);
    uint32_t varying_count = r_data->r_vertex_shader->varying_count;

    // Number of elements in the primitive buffer for a single vertex
    const uint32_t vsib = varying_count + 1;

    // Contains the vertices of the currently proccesed primitive
    mcc_vec4f *current_primitive_buffer = calloc(vsib * 3, sizeof(mcc_vec4f));

    struct mcc_cpurast_vertex_shader_input vert_input = {
        .o_in_data = r_data->o_vertex_shader_data,
    };

    uint32_t vertex_idx = r_data->vertex_start_idx;
    uint32_t vertex_index_increment;
    
    switch (r_data->vertex_processing) {
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
        vertex_index_increment = 3;
        break;
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
        // If we are in triangle strip, the first two vertices must be processed before
        // for the next primitives, the first two vertices will be the ones from the
        // previous primitive.
        vertex_index_increment = 1;
        for (size_t di = 0; di < 2; di++, vertex_idx++) {
            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = (union mcc_cpurast_shaders_varying*)
                &current_primitive_buffer[vsib * di + 1];
            r_data->r_vertex_shader->r_fn(&vert_input);
            current_primitive_buffer[vsib * di] = vert_input.out_position;
        }
        break;
    }
    uint32_t output_vertex_counter = 0;

    for (; vertex_idx < r_data->vertex_start_idx + r_data->vertex_count; vertex_idx += vertex_index_increment) {
        switch (r_data->vertex_processing) {
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
            for (uint32_t di = 0; di < 3; di++) {
                vert_input.in_vertex_idx = vertex_idx + di;
                vert_input.r_out_varyings = (union mcc_cpurast_shaders_varying*)
                    &current_primitive_buffer[vsib * di + 1];
                r_data->r_vertex_shader->r_fn(&vert_input);
                current_primitive_buffer[vsib * di] = vert_input.out_position;
            }
            break;
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
            // 'move' the last two vertices on the first one
            memmove(
                &current_primitive_buffer[0],
                &current_primitive_buffer[vsib],
                // The last two vertex
                sizeof(mcc_vec4f) * vsib * 2
            );

            // Compute the last vertex only
            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = (union mcc_cpurast_shaders_varying*)
                &current_primitive_buffer[vsib * 2 + 1];
            r_data->r_vertex_shader->r_fn(&vert_input);
            current_primitive_buffer[vsib * 2] = vert_input.out_position;
            break;
        }

        memcpy(
            &r_data->r_out_primitive_buffer[output_vertex_counter * vsib],
            current_primitive_buffer,
            sizeof(mcc_vec4f) * vsib * 3
        );
        
        struct mcc_triangle_clip_context clip_ctx = {
            .varying_count = varying_count,
            .r_primitive_buffer = &r_data->r_out_primitive_buffer[output_vertex_counter * vsib],
        };
        clip_triangle(&clip_ctx);

        output_vertex_counter += clip_ctx.out_vertex_count;
    }

    r_data->out_vertex_count = output_vertex_counter;

    free(current_primitive_buffer);
    if (r_data->o_wait_counter)
        mcc_wait_counter_decrement(r_data->o_wait_counter, 1);
}

void mcc_cpurast_render(const struct mcc_cpurast_render_config *r_config) {
    assert(r_config->r_fragment_shader->varying_count == r_config->r_vertex_shader->varying_count);
    uint32_t varying_count = r_config->r_vertex_shader->varying_count;

    // TODO: For very big meshes if would make sense to break it down into
    //       batches?
    // One element for vertex position, and one for each additional varying,
    // and all of that for each vertex (pretty big!)
    size_t primitive_buffer_size =
        sizeof(mcc_vec4f) * (varying_count + 1) * r_config->vertex_count * MAX_SUBTRIANGLES;
    fprintf(stderr, "primitive_buffer_size: %zu bytes\n", primitive_buffer_size);
    mcc_vec4f *primitive_buffer = malloc(primitive_buffer_size);

    // Create one task for each 32 triangles
    const uint32_t vertex_task_size = 3 * 32;
    const uint32_t vertex_task_count = mcc_up_div(r_config->vertex_count, vertex_task_size);
    printf("vertex_task_count: %u\n", vertex_task_count);
    struct vertex_process_task_data *tasks_data = malloc(sizeof(*tasks_data) * vertex_task_count);
    
    struct mcc_wait_counter vertex_processing_wait_counter;
    mcc_wait_counter_init(&vertex_processing_wait_counter, vertex_task_count);

    for (uint32_t buffer_offset = 0, task_idx = 0; task_idx < vertex_task_count; task_idx++) {
        tasks_data[task_idx] = (struct vertex_process_task_data) {
            .o_fragment_shader_data = r_config->o_fragment_shader_data,
            .r_fragment_shader = r_config->r_fragment_shader,
            .o_vertex_shader_data = r_config->o_vertex_shader_data,
            .r_vertex_shader = r_config->r_vertex_shader,

            .culling_mode = r_config->culling_mode,
            .vertex_processing = r_config->vertex_processing,

            .vertex_start_idx = task_idx * vertex_task_size,
            .vertex_count = vertex_task_size,

            .o_wait_counter = &vertex_processing_wait_counter,

            .r_out_primitive_buffer = primitive_buffer,
            .out_vertex_count = ~0u,
        };
        // Make sure we do not overflow
        tasks_data[task_idx].vertex_count = mcc_min(
            tasks_data[task_idx].vertex_count,
            r_config->vertex_count - tasks_data[task_idx].vertex_start_idx
        );
        assert(tasks_data->vertex_count > 0);

        // assigns to this task only part of the output buffer
        tasks_data[task_idx].r_out_primitive_buffer += buffer_offset;
        // this is the maximum amount of vertices this task can output
        buffer_offset += (varying_count + 1) * MAX_SUBTRIANGLES * tasks_data[task_idx].vertex_count;
        assert(buffer_offset <= (varying_count + 1) * r_config->vertex_count * MAX_SUBTRIANGLES);
    }

    struct mcc_thread_pool *pool = mcc_thread_pool_global();
    mcc_thread_pool_lock(pool);
    for (uint32_t task_idx = 0; task_idx < vertex_task_count; task_idx++) {
        mcc_thread_pool_push_task(pool, (struct mcc_thread_pool_task){
            .data = &tasks_data[task_idx],
            .fn = vertex_process_task,
        });
    }
    mcc_thread_pool_unlock(pool);

    mcc_wait_counter_wait(&vertex_processing_wait_counter);
    mcc_wait_counter_free(&vertex_processing_wait_counter);

    uint32_t vertex_index_increment;
    switch (r_config->vertex_processing) {
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
        vertex_index_increment = 3;
        break;
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
        vertex_index_increment = 1;
        break;
    }

    // Make an array of array for easy indexing
    auto primitive_buffer_arr = (mcc_vec4f (*)[varying_count+1])primitive_buffer;

    primitive_t primitive = { .vertices = {
        { .varyings = calloc(varying_count, sizeof(mcc_vec4f)) },
        { .varyings = calloc(varying_count, sizeof(mcc_vec4f)) },
        { .varyings = calloc(varying_count, sizeof(mcc_vec4f)) },
    }};
    union mcc_cpurast_shaders_varying *fragment_varyings = calloc(varying_count, sizeof(mcc_vec4f));

    struct mcc_cpurast_fragment_shader_input frag_input = {
        .o_in_data = r_config->o_fragment_shader_data,
        .r_in_varyings = fragment_varyings,
    };
    struct mcc_rasterization_context rasterizaton_context = {
        .culling_mode = r_config->culling_mode,
        .r_attachment = r_config->r_attachment,
        .r_primitive = &primitive,
        .r_fragment_varyings = fragment_varyings,
        .r_frag_input = &frag_input,
        .r_fragment_shader = r_config->r_fragment_shader,
        .o_depth_comparison_fn = r_config->o_depth_comparison_fn,
        .varying_count = varying_count,
    };

    for (uint32_t task_idx = 0; task_idx < vertex_task_count; task_idx++) {
        const uint32_t task_buffer_start = safe_to_u32(
            tasks_data[task_idx].r_out_primitive_buffer - primitive_buffer
        ) / (varying_count + 1);
        const uint32_t task_buffer_size = tasks_data[task_idx].out_vertex_count;
        const uint32_t task_buffer_end = task_buffer_start + task_buffer_size;

        for (uint32_t vertex_idx = task_buffer_start; vertex_idx < task_buffer_end; vertex_idx += vertex_index_increment) {
            for (uint32_t sub_vert_i = 0; sub_vert_i < 3; sub_vert_i++) {
                mcc_vec4f *vs = primitive_buffer_arr[vertex_idx + sub_vert_i];
                primitive.vertices[sub_vert_i].pos_homogeneous = vs[0];
                primitive.vertices[sub_vert_i].w_inv = 1.f / vs[0].w;
                for (uint32_t varying_i = 0; varying_i < varying_count; varying_i++) {
                    primitive.vertices[sub_vert_i].varyings[varying_i].vec4f = vs[1 + varying_i];
                }
            }

            rasterize_triangle(&rasterizaton_context);
        }
    }

    for (size_t i = 0; i < 3; i++) {
        free(primitive.vertices[i].varyings);
    }
    free(fragment_varyings);
    free(primitive_buffer);
}
