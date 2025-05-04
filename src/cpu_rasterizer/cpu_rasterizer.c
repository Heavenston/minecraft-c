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

/**
 * Computes `det(v1 - v0, point - v0) > 0.f`
 * Used to test point lies on the left side of a line
 */
static bool check_point(mcc_vec2f p, mcc_vec2f v0, mcc_vec2f v1) {
    mcc_vec2f c0 = mcc_vec2f_sub(v1, v0);
    
    mcc_vec2f c1 = mcc_vec2f_sub(p, v0);
    float det = mcc_mat2f_det(mcc_mat2f_col(c0, c1));

    return det > 0.f;
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
     * Weight for v0 (calculated as 1 - u - v)
     */
    float w;
    /**
     * True if the point is inside or on the boundary
     */
    bool isInside;
};

/**
 * Calculates Barycentric coordinates (u, v, w) for screenPos relative to
 * triangle (v0, v1, v2).
 * P = w*v0 + u*v1 + v*v2
 * where u >= 0, v >= 0, w >= 0 (or u + v <= 1) for points inside the triangle.
 */
static struct mcc_barycentric_coords mcc_calculate_barycentric(mcc_vec2f v0, mcc_vec2f v1, mcc_vec2f v2, mcc_vec2f screenPos) {
    struct mcc_barycentric_coords result = {};
    // Calculate edge vectors and vector from v0 to the point
    mcc_vec2f v0v1 = mcc_vec2f_sub(v1, v0); // Vector A in P - v0 = u*A + v*B
    mcc_vec2f v0v2 = mcc_vec2f_sub(v2, v0); // Vector B
    mcc_vec2f v0p  = mcc_vec2f_sub(screenPos, v0); // Vector C

    // Calculate dot products needed for the system of equations:
    // dot(C, A) = u * dot(A, A) + v * dot(B, A)
    // dot(C, B) = u * dot(A, B) + v * dot(B, B)
    float dot00 = mcc_vec2f_dot(v0v1, v0v1); // dot(A, A)
    float dot01 = mcc_vec2f_dot(v0v1, v0v2); // dot(A, B) == dot(B, A)
    float dot11 = mcc_vec2f_dot(v0v2, v0v2); // dot(B, B)
    float dot20 = mcc_vec2f_dot(v0p,  v0v1); // dot(C, A)
    float dot21 = mcc_vec2f_dot(v0p,  v0v2); // dot(C, B)

    // Calculate denominator for Cramer's rule or matrix inversion
    // Denom = dot(A, A) * dot(B, B) - dot(A, B) * dot(B, A)
    float denom = dot00 * dot11 - dot01 * dot01;

    // Check if the triangle is degenerate (collinear vertices)
    // Use a small epsilon for floating-point comparison
    float epsilon = 1e-7f;
    if (denom < epsilon && denom > -epsilon) {
        // Triangle is degenerate, cannot reliably calculate coordinates.
        // Return default values (outside).
        return result;
    }

    // Calculate u and v using Cramer's rule (or by solving the 2x2 system)
    // u = (dot(B, B) * dot(C, A) - dot(A, B) * dot(C, B)) / Denom
    // v = (dot(A, A) * dot(C, B) - dot(A, B) * dot(C, A)) / Denom
    float invDenom = 1.f / denom;
    result.u = (dot11 * dot20 - dot01 * dot21) * invDenom;
    result.v = (dot00 * dot21 - dot01 * dot20) * invDenom;

    // Calculate w using the property that coordinates sum to 1
    result.w = 1.f - result.u - result.v;

    // Check if the point is inside the triangle (including boundaries)
    // All coordinates must be non-negative.
    // Due to floating point inaccuracies, allow a small tolerance (epsilon).
    if (result.u >= -epsilon && result.v >= -epsilon && result.w >= -epsilon) {
    // Alternatively: if (result.u >= -epsilon && result.v >= -epsilon && (result.u + result.v) <= 1.f + epsilon) {
        result.isInside = true;
    } else {
        result.isInside = false; // Explicitly set to false if outside
    }


    return result;
}

struct processed_vertex {
    union mcc_cpurast_shaders_varying *varyings;
    mcc_vec3f pos3;
    mcc_vec4f pos4;
};

struct mcc_pixel_params {
    struct mcc_barycentric_coords barycentric;
    mcc_vec2f screen_pos;
    float depth;
    size_t pixel_idx;
};

typedef bool (*mcc_polygon_filter_fn)(const struct mcc_barycentric_coords *coords);

struct mcc_rasterization_context {
    struct mcc_cpurast_rendering_attachment *r_attachment;
    const struct processed_vertex *r_vertices;
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
    
    int x0 = (int)(((v0->pos3.x + 1.f) * 0.5f * wf) + 0.5f);
    int y0 = (int)(((-v0->pos3.y + 1.f) * 0.5f * hf) + 0.5f);
    int x1 = (int)(((v1->pos3.x + 1.f) * 0.5f * wf) + 0.5f);
    int y1 = (int)(((-v1->pos3.y + 1.f) * 0.5f * hf) + 0.5f);

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
            
            int origX0 = (int)(((v0->pos3.x + 1.f) * 0.5f * wf) + 0.5f);
            int origY0 = (int)(((-v0->pos3.y + 1.f) * 0.5f * hf) + 0.5f);
            
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
                .isInside = true
            };
            
            float depth = v0->pos3.z * (1.0f - t) + v1->pos3.z * t;
            
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

void process_vertex_init(struct processed_vertex *v, size_t varying_count) {
    v->varyings = calloc(varying_count, sizeof(mcc_vec4f));
}

void process_vertex_free(struct processed_vertex *v) {
    free(v->varyings);
}

// Helper function to process a single fragment
static void process_fragment(const struct mcc_rasterization_context *r_context, const struct mcc_pixel_params *pixel) {
    auto depth_attachment = r_context->r_attachment->o_depth;
    auto color_attachment = r_context->r_attachment->o_color;
    const struct processed_vertex *vertices = r_context->r_vertices;
    const struct mcc_barycentric_coords *barycentric = &pixel->barycentric;
    
    // Interpolate varying attributes using barycentric coordinates
    for (size_t varying_i = 0; varying_i < r_context->varying_count; varying_i++) {
        auto varv1 = mcc_vec4f_scale(vertices[1].varyings[varying_i].vec4f, barycentric->u);
        auto varv2 = mcc_vec4f_scale(vertices[2].varyings[varying_i].vec4f, barycentric->v);
        auto varv0 = mcc_vec4f_scale(vertices[0].varyings[varying_i].vec4f, barycentric->w);
        r_context->r_fragment_varyings[varying_i].vec4f = mcc_vec4f_add(varv0, mcc_vec4f_add(varv1, varv2));
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
    const struct processed_vertex *vertices = r_context->r_vertices;
    
    if (r_context->polygon_filter == polygon_filter_wireframe) {
        draw_line(r_context, &vertices[0], &vertices[1]);
        draw_line(r_context, &vertices[1], &vertices[2]);
        draw_line(r_context, &vertices[2], &vertices[0]);
        return;
    }

    // Get vertex positions
    mcc_vec3f v0 = vertices[0].pos3,
              v1 = vertices[1].pos3,
              v2 = vertices[2].pos3;
    mcc_vec2f p0 = v0.xy,
              p1 = v1.xy,
              p2 = v2.xy;

    // Screen dimensions as floats
    float wf = (float)r_context->r_attachment->width;
    float hf = (float)r_context->r_attachment->height;

    // AABB of the triangle in viewport coordinate
    float min_xf = min3f(p0.x, p1.x, p2.x);
    float min_yf = min3f(p0.y, p1.y, p2.y);
    float max_xf = max3f(p0.x, p1.x, p2.x);
    float max_yf = max3f(p0.y, p1.y, p2.y);

    // AABB of the triangle in screen coordinate
    // NOTE: Since y is inverted we need to take the min pos to have the max pixel
    uint32_t min_x = (uint32_t)(( min_xf + 1.f) * 0.5f * wf - 0.5f);
    uint32_t min_y = (uint32_t)((-max_yf + 1.f) * 0.5f * hf - 0.5f);
    uint32_t max_x = (uint32_t)(( max_xf + 1.f) * 0.5f * wf + 0.5f);
    uint32_t max_y = (uint32_t)((-min_yf + 1.f) * 0.5f * hf + 0.5f);

    // Iterate over pixels
    for (uint32_t y = min_y; y < max_y; y++) {
        for (uint32_t x = min_x; x < max_x; x++) {
            size_t pixel_idx = x + y * r_context->r_attachment->width;

            float fx = (float)x + 0.5f;
            float fy = (float)y + 0.5f;

            // Normalized screen coordinates
            mcc_vec2f screen_pos = {{
                (fx / wf) * 2.f - 1.f,
                (fy / hf) *-2.f + 1.f,
            }};

            // Calculate barycentric coordinates
            auto barycentric = mcc_calculate_barycentric(p0, p1, p2, screen_pos);
            if (!barycentric.isInside)
                continue;
            
            // Apply polygon mode filter (fill, point)
            if (!r_context->polygon_filter(&barycentric))
                continue;

            // Calculate depth using barycentric coordinates
            float depth = v1.z * barycentric.u + v2.z * barycentric.v + v0.z * barycentric.w;

            // Early depth test
            if (depth < 0. || depth > 1.) {
                continue;
            }

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

    const size_t vertex_per_primitive = 3;
    struct processed_vertex vertices[3];
    for (size_t i = 0; i < vertex_per_primitive; i++) {
        process_vertex_init(&vertices[i], varying_count);
    }
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
            struct processed_vertex *pv = &vertices[di + 1];

            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = pv->varyings;
            r_config->r_vertex_shader->r_fn(&vert_input);
            pv->pos4 = vert_input.out_position;
            pv->pos3 = mcc_vec3f_scale(pv->pos4.xyz, 1.f / pv->pos4.w);
        }
        break;
    default:
        // NOTE: Silence 'vertex_index_increment' may be uninitialized warning.
        vertex_index_increment = 3;
        abort();
    }

    for (; vertex_idx < r_config->vertex_count; vertex_idx += vertex_index_increment) {
        switch (r_config->vertex_processing) {
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
            for (size_t di = 0; di < 3; di++) {
                vert_input.in_vertex_idx = vertex_idx + safe_to_u32(di);
                vert_input.r_out_varyings = vertices[di].varyings;
                r_config->r_vertex_shader->r_fn(&vert_input);
                vertices[di].pos4 = vert_input.out_position;
                vertices[di].pos3 = mcc_vec3f_scale(vertices[di].pos4.xyz, 1.f / vertices[di].pos4.w);
            }
            break;
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_STRIP:
            struct processed_vertex first = vertices[0];
            vertices[0] = vertices[1];
            vertices[1] = vertices[2];
            vertices[2] = first;

            vert_input.in_vertex_idx = vertex_idx;
            vert_input.r_out_varyings = vertices[2].varyings;
            r_config->r_vertex_shader->r_fn(&vert_input);
            vertices[2].pos4 = vert_input.out_position;
            vertices[2].pos3 = mcc_vec3f_scale(vertices[2].pos4.xyz, 1.f / vertices[2].pos4.w);
            break;
        }

        mcc_vec2f p0 = vertices[0].pos3.xy,
                  p1 = vertices[1].pos3.xy,
                  p2 = vertices[2].pos3.xy;

        bool isCcw = check_point(p2, p0, p1);

        if (r_config->culling_mode == MCC_CPURAST_CULLING_MODE_CW && !isCcw)
            continue;
        if (r_config->culling_mode == MCC_CPURAST_CULLING_MODE_CCW && isCcw)
            continue;

        // Create rasterization context
        struct mcc_rasterization_context context = {
            .r_attachment = r_config->r_attachment,
            .r_vertices = vertices,
            .r_fragment_varyings = fragment_varyings,
            .r_frag_input = &frag_input,
            .r_fragment_shader = r_config->r_fragment_shader,
            .o_depth_comparison_fn = r_config->o_depth_comparison_fn,
            .varying_count = varying_count,
            .polygon_filter = polygon_filter
        };
        
        // Call the unified rasterization function
        rasterize_triangle(&context);
    }

    for (size_t i = 0; i < vertex_per_primitive; i++) {
        process_vertex_free(&vertices[i]);
    }
    free(fragment_varyings);
}
