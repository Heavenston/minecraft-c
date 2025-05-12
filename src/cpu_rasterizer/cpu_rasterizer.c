#include "cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/vector.h"
#include "linalg/scalars.h"
#include "safe_cast.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Like processed_vertex but for storage inside congiguous primitive buffer
 */
struct processed_vertex_inline {
    mcc_vec4f pos_homogeneous;
    mcc_vec2f pos_clipspace;
    /**
     * Flexible array member for the n number of varyings
     */
    mcc_vec4f varyings[];
};

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

struct clip_triangle_output {
    /**
     * Between 0 and MAX_SUBTRIANGLES (included)
     */
    size_t triangle_count;
    primitive_t triangles[MAX_SUBTRIANGLES];
};

void clipped_triangles_append(struct clip_triangle_output *into, struct clip_triangle_output from) {
    assert(into->triangle_count + from.triangle_count <= MAX_SUBTRIANGLES);
    for (size_t i = 0; i < from.triangle_count; i++) {
        into->triangles[into->triangle_count++] = from.triangles[i];
    }
}

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

struct clip_triangle_context {
    size_t varyings_count;
    preallocated_varyings_t *preallocated_varyings;
    primitive_t triangle;
    struct plane plane;
};

typedef struct intersection_context {
    struct clip_triangle_context *ctctx;
    union mcc_cpurast_shaders_varying *touse_varyings;
    struct processed_vertex v0;
    struct processed_vertex v1;
    float value0;
    float value1;
} ic_t;

static struct processed_vertex clip_edge(ic_t ctx) {
    float t = ctx.value0 / (ctx.value0 - ctx.value1);

    struct processed_vertex v;
    v.pos_homogeneous = mcc_vec4f_add(
        mcc_vec4f_scale(ctx.v0.pos_homogeneous, 1.f - t),
        mcc_vec4f_scale(ctx.v1.pos_homogeneous, t)
    );
    v.w_inv = 1.f / v.pos_homogeneous.w;
    // TODO: Use pre allocated values (this is a leak)
    v.varyings = ctx.touse_varyings;
    for (size_t vi = 0; vi < ctx.ctctx->varyings_count; vi++) {
        v.varyings[vi].vec4f = mcc_vec4f_add(
            mcc_vec4f_scale(ctx.v0.varyings[vi].vec4f, 1.f - t),
            mcc_vec4f_scale(ctx.v1.varyings[vi].vec4f, t)
        );
    }

    return v;
}

/*
 * Adapted from
 * https://www.gabrielgambetta.com/computer-graphics-from-scratch/11-clipping.html
 * And
 * https://lisyarus.github.io/blog/posts/implementing-a-tiny-cpu-rasterizer-part-5.html
 */
static struct clip_triangle_output clip_triangle_once(struct clip_triangle_context ctx) {
    primitive_t triangle = ctx.triangle;
    struct plane plane = ctx.plane;
    struct clip_triangle_output output = {};

    float values[3] = {
        signed_distance(plane, triangle.v0.pos_homogeneous),
        signed_distance(plane, triangle.v1.pos_homogeneous),
        signed_distance(plane, triangle.v2.pos_homogeneous)
    };

    uint8_t mask = (values[0] <= 0.f ? 1 : 0) | (values[1] <= 0.f ? 2 : 0) | (values[2] <= 0.f ? 4 : 0);

    // Preemptively allocate two varyings array, but decrement the free_head if they
    // end up not being used, kinda ugly but i dont want to have these 2 lines
    // 6 times
    auto nva = ctx.preallocated_varyings->v[ctx.preallocated_varyings->free_head++];
    auto nvb = ctx.preallocated_varyings->v[ctx.preallocated_varyings->free_head++];

    switch (mask) {
    case 0b000:
        output.triangles[output.triangle_count] = triangle;
        output.triangle_count++;
        ctx.preallocated_varyings->free_head -= 2;
        break;
    case 0b001: {
        struct processed_vertex v01 = clip_edge((ic_t){ &ctx, nva, triangle.v0, triangle.v1, values[0], values[1] });
        struct processed_vertex v02 = clip_edge((ic_t){ &ctx, nvb, triangle.v0, triangle.v2, values[0], values[2] });

        output.triangles[output.triangle_count].v0 = v01;
        output.triangles[output.triangle_count].v1 = triangle.v1;
        output.triangles[output.triangle_count].v2 = triangle.v2;
        output.triangle_count++;

        output.triangles[output.triangle_count].v0 = v01;
        output.triangles[output.triangle_count].v1 = triangle.v2;
        output.triangles[output.triangle_count].v2 = v02;
        output.triangle_count++;
        break;
    }
    case 0b010: {
        struct processed_vertex v10 = clip_edge((ic_t){ &ctx, nva, triangle.v1, triangle.v0, values[1], values[0] });
        struct processed_vertex v12 = clip_edge((ic_t){ &ctx, nvb, triangle.v1, triangle.v2, values[1], values[2] });

        output.triangles[output.triangle_count].v0 = triangle.v0;
        output.triangles[output.triangle_count].v1 = v10;
        output.triangles[output.triangle_count].v2 = triangle.v2;
        output.triangle_count++;

        output.triangles[output.triangle_count].v0 = triangle.v2;
        output.triangles[output.triangle_count].v1 = v10;
        output.triangles[output.triangle_count].v2 = v12;
        output.triangle_count++;
        break;
    }
    case 0b011: {
        struct processed_vertex v02 = clip_edge((ic_t){ &ctx, nva, triangle.v0, triangle.v2, values[0], values[2] });
        struct processed_vertex v12 = clip_edge((ic_t){ &ctx, nvb, triangle.v1, triangle.v2, values[1], values[2] });

        output.triangles[output.triangle_count].v0 = v02;
        output.triangles[output.triangle_count].v1 = v12;
        output.triangles[output.triangle_count].v2 = triangle.v2;
        output.triangle_count++;
        break;
    }
    case 0b100: {
        struct processed_vertex v20 = clip_edge((ic_t){ &ctx, nva, triangle.v2, triangle.v0, values[2], values[0] });
        struct processed_vertex v21 = clip_edge((ic_t){ &ctx, nvb, triangle.v2, triangle.v1, values[2], values[1] });

        output.triangles[output.triangle_count].v0 = triangle.v0;
        output.triangles[output.triangle_count].v1 = triangle.v1;
        output.triangles[output.triangle_count].v2 = v20;
        output.triangle_count++;

        output.triangles[output.triangle_count].v0 = v20;
        output.triangles[output.triangle_count].v1 = triangle.v1;
        output.triangles[output.triangle_count].v2 = v21;
        output.triangle_count++;
        break;
    }
    case 0b101: {
        struct processed_vertex v01 = clip_edge((ic_t){ &ctx, nva, triangle.v0, triangle.v1, values[0], values[1] });
        struct processed_vertex v21 = clip_edge((ic_t){ &ctx, nvb, triangle.v2, triangle.v1, values[2], values[1] });

        output.triangles[output.triangle_count].v0 = v01;
        output.triangles[output.triangle_count].v1 = triangle.v1;
        output.triangles[output.triangle_count].v2 = v21;
        output.triangle_count++;
        break;
    }
    case 0b110: {
        struct processed_vertex v10 = clip_edge((ic_t){ &ctx, nva, triangle.v1, triangle.v0, values[1], values[0] });
        struct processed_vertex v20 = clip_edge((ic_t){ &ctx, nvb, triangle.v2, triangle.v0, values[2], values[0] });

        output.triangles[output.triangle_count].v0 = triangle.v0;
        output.triangles[output.triangle_count].v1 = v10;
        output.triangles[output.triangle_count].v2 = v20;
        output.triangle_count++;
        break;
    }
    case 0b111:
        ctx.preallocated_varyings->free_head -= 2;
        break;
    }

    assert(ctx.preallocated_varyings->free_head <= PREALLOCATED_VARYINGS_SIZE);
    return output;
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

struct mcc_triangle_clip_context {
    primitive_t *r_primitive;
    size_t varying_count;
    preallocated_varyings_t *r_preallocated_varyings;
};

struct mcc_rasterization_context {
    enum mcc_cpurast_culling_mode culling_mode;
    preallocated_varyings_t *r_preallocated_varyings;
    struct mcc_cpurast_rendering_attachment *r_attachment;
    primitive_t *r_primitive;
    union mcc_cpurast_shaders_varying *r_fragment_varyings;
    struct mcc_cpurast_fragment_shader_input *r_frag_input;
    struct mcc_fragment_shader *r_fragment_shader;
    mcc_depth_comparison_fn o_depth_comparison_fn;
    size_t varying_count;
    mcc_polygon_filter_fn polygon_filter;
};

typedef void (*mcc_rasterize_triangle_fn)(const struct mcc_rasterization_context*);

// Forward declaration
static void process_fragment(const struct mcc_rasterization_context *r_context, const struct mcc_pixel_params *pixel);

static bool polygon_filter_fill(const struct mcc_barycentric_coords *) {
    // All pixels are rendered
    return true;
}

static void draw_line(const struct mcc_rasterization_context *r_context, 
                     const struct processed_vertex *v0,
                     const struct processed_vertex *v1) {
    float wf = (float)r_context->r_attachment->width;
    float hf = (float)r_context->r_attachment->height;

    mcc_vec3f v0_pos_euclidian = mcc_vec3f_scale(v0->pos_homogeneous.xyz, v0->w_inv);
    mcc_vec3f v1_pos_euclidian = mcc_vec3f_scale(v1->pos_homogeneous.xyz, v1->w_inv);
    
    int x0 = (int)(((v0_pos_euclidian.x + 1.f) * 0.5f * wf) + 0.5f);
    int y0 = (int)(((-v0_pos_euclidian.y + 1.f) * 0.5f * hf) + 0.5f);
    int x1 = (int)(((v1_pos_euclidian.x + 1.f) * 0.5f * wf) + 0.5f);
    int y1 = (int)(((-v1_pos_euclidian.y + 1.f) * 0.5f * hf) + 0.5f);

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true) {
        if (x0 >= 0 && x0 < (int)r_context->r_attachment->width && 
            y0 >= 0 && y0 < (int)r_context->r_attachment->height) {
            size_t pixel_idx = (size_t)x0 + (size_t)y0 * r_context->r_attachment->width;
            
            mcc_vec2f screen_pos = {{
                ((float)x0 + 0.5f) / wf * 2.f - 1.f,
                ((float)y0 + 0.5f) / hf *-2.f + 1.f,
            }};
            
            int origX0 = (int)(((v0_pos_euclidian.x + 1.f) * 0.5f * wf) + 0.5f);
            int origY0 = (int)(((-v0_pos_euclidian.y + 1.f) * 0.5f * hf) + 0.5f);
            
            float currentDistSq = (float)((x0 - origX0) * (x0 - origX0) + (y0 - origY0) * (y0 - origY0));
            float totalDistSq = (float)((x1 - origX0) * (x1 - origX0) + (y1 - origY0) * (y1 - origY0));
            
            float t;
            if (totalDistSq > 0.0f) {
                t = sqrtf(currentDistSq / totalDistSq);
            } else {
                t = 0.0f;
            }
            
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            
            struct mcc_barycentric_coords barycentric = {
                .u = t,
                .v = 0.0f,
                .w = 1.0f - t,
            };
            
            float depth = v0_pos_euclidian.z * (1.0f - t) + v1_pos_euclidian.z * t;
            
            auto depth_attachment = r_context->r_attachment->o_depth;
            if (depth < 0.f || depth > 1.f) {
            } else if (
                depth_attachment &&
                r_context->o_depth_comparison_fn &&
                !r_context->o_depth_comparison_fn(depth_attachment->r_data[pixel_idx], depth)
            ) {
            } else {
                struct mcc_pixel_params pixel = {
                    .barycentric = barycentric,
                    .screen_pos = screen_pos,
                    .depth = depth,
                    .pixel_idx = pixel_idx
                };
                process_fragment(r_context, &pixel);
            }
        }
        
        if (x0 == x1 && y0 == y1) break;
        
        e2 = 2 * err;
        if (e2 >= dy) {
            if (x0 == x1) break;
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            if (y0 == y1) break;
            err += dx;
            y0 += sy;
        }
    }
}

static bool polygon_filter_wireframe(const struct mcc_barycentric_coords *) {
    return false;
}

static bool polygon_filter_point(const struct mcc_barycentric_coords *coords) {
    // Only pixels near vertices are rendered
    const float point_threshold = 0.05f;
    return coords->u >= (1.0f - point_threshold) || 
           coords->v >= (1.0f - point_threshold) || 
           coords->w >= (1.0f - point_threshold);
}

// Helper function to process a single fragment
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
    float det012 = mcc_mat2f_det(mcc_mat2f_col(mcc_vec2f_sub(p1, p0), mcc_vec2f_sub(p2, p0)));

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
    
    if (r_context->polygon_filter == polygon_filter_wireframe) {
        draw_line(r_context, &primitive->v0, &primitive->v1);
        draw_line(r_context, &primitive->v1, &primitive->v2);
        draw_line(r_context, &primitive->v2, &primitive->v0);
        return;
    }

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
            
            // Apply polygon mode filter (fill, point)
            if (!r_context->polygon_filter(&barycentric))
                continue;

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

static struct clip_triangle_output clip_triangle(struct mcc_triangle_clip_context ctx) {
    struct clip_triangle_output output = {
        .triangle_count = 1,
        .triangles = { *ctx.r_primitive },
    };

    for (size_t plane_i = 0; plane_i < sizeof(clipping_planes) / sizeof(*clipping_planes); plane_i++) {
        struct clip_triangle_output sub_output = {};

        for (size_t primitive_i = 0; primitive_i < output.triangle_count; primitive_i++) {
            auto clip_output = clip_triangle_once((struct clip_triangle_context){
                .varyings_count = ctx.varying_count,
                .preallocated_varyings = ctx.r_preallocated_varyings,
                .triangle = output.triangles[primitive_i],
                .plane = clipping_planes[plane_i],
            });
            clipped_triangles_append(&sub_output, clip_output);
        }

        output = sub_output;
    }

    return output;
}

void mcc_cpurast_render(const struct mcc_cpurast_render_config *r_config) {
    assert(r_config->r_fragment_shader->varying_count == r_config->r_vertex_shader->varying_count);
    size_t varying_count = r_config->r_vertex_shader->varying_count;
    
    // Select polygon filter function based on polygon mode
    mcc_polygon_filter_fn polygon_filter;
    switch (r_config->polygon_mode) {
        case MCC_CPURAST_POLYGON_MODE_LINE:
            polygon_filter = polygon_filter_wireframe;
            break;
        case MCC_CPURAST_POLYGON_MODE_POINT:
            polygon_filter = polygon_filter_point;
            break;
        case MCC_CPURAST_POLYGON_MODE_FILL:
        default:
            polygon_filter = polygon_filter_fill;
            break;
    }

    preallocated_varyings_t preallocated_varyings;
    for (size_t i = 0; i < PREALLOCATED_VARYINGS_SIZE; i++) {
        preallocated_varyings.v[i] = calloc(varying_count, sizeof(mcc_vec4f));
    }

    primitive_t primitive = { .vertices = {
        { .varyings = preallocated_varyings.v[0] },
        { .varyings = preallocated_varyings.v[1] },
        { .varyings = preallocated_varyings.v[2] },
    }};
    union mcc_cpurast_shaders_varying *fragment_varyings = calloc(varying_count, sizeof(mcc_vec4f));

    struct mcc_cpurast_fragment_shader_input frag_input = {
        .o_in_data = r_config->o_fragment_shader_data,
        .r_in_varyings = fragment_varyings,
    };
    struct mcc_cpurast_vertex_shader_input vert_input = {
        .o_in_data = r_config->o_vertex_shader_data,
    };

    uint32_t vertex_idx = 0;
    uint32_t vertex_index_increment;

    // If we are in triangle strip, the first two vertices must be processed before
    // for the next primitives, the first two vertices will be the ones from the
    // previous primitive.
    switch (r_config->vertex_processing) {
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
        vertex_index_increment = 3;
        break;
    case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
        vertex_index_increment = 1;
        for (size_t di = 0; di < 2; di++, vertex_idx++) {
            struct processed_vertex *pv = &primitive.vertices[di + 1];

            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = pv->varyings;
            r_config->r_vertex_shader->r_fn(&vert_input);
            pv->pos_homogeneous = vert_input.out_position;
            pv->w_inv = 1.f / pv->pos_homogeneous.w;
        }
        break;
    }

    // Create rasterization context
    struct mcc_rasterization_context context = {
        .culling_mode = r_config->culling_mode,
        .r_preallocated_varyings = &preallocated_varyings,
        .r_attachment = r_config->r_attachment,
        .r_primitive = &primitive,
        .r_fragment_varyings = fragment_varyings,
        .r_frag_input = &frag_input,
        .r_fragment_shader = r_config->r_fragment_shader,
        .o_depth_comparison_fn = r_config->o_depth_comparison_fn,
        .varying_count = varying_count,
        .polygon_filter = polygon_filter
    };

    for (; vertex_idx < r_config->vertex_count; vertex_idx += vertex_index_increment) {
        switch (r_config->vertex_processing) {
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
            for (size_t di = 0; di < 3; di++) {
                vert_input.in_vertex_idx = vertex_idx + safe_to_u32(di);
                vert_input.r_out_varyings = primitive.vertices[di].varyings;
                r_config->r_vertex_shader->r_fn(&vert_input);
                primitive.vertices[di].pos_homogeneous = vert_input.out_position;
                primitive.vertices[di].w_inv = 1.f / primitive.vertices[di].pos_homogeneous.w;
            }
            break;
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
            // 'rotate' the vertices to put the last two as the first two
            auto v0_copy = primitive.v0;
            primitive.v0 = primitive.v1;
            primitive.v1 = primitive.v2;
            primitive.v2 = v0_copy;

            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = primitive.v2.varyings;
            r_config->r_vertex_shader->r_fn(&vert_input);
            primitive.v2.pos_homogeneous = vert_input.out_position;
            primitive.v2.w_inv = 1.f / primitive.v2.pos_homogeneous.w;
            break;
        }
        
        preallocated_varyings.free_head = 3;
        auto clipped_output = clip_triangle((struct mcc_triangle_clip_context) {
            .r_primitive = &primitive,
            .varying_count = varying_count,
            .r_preallocated_varyings = &preallocated_varyings,
        });

        for (size_t si = 0; si < clipped_output.triangle_count; si++) {
            context.r_primitive = &clipped_output.triangles[si];
            rasterize_triangle(&context);
        }
    }

    for (size_t i = 0; i < PREALLOCATED_VARYINGS_SIZE; i++) {
        free(preallocated_varyings.v[i]);
    }
    free(fragment_varyings);
}
