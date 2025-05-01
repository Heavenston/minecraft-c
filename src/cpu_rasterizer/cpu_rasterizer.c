#include "cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/vector.h"

#include <assert.h>
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

void process_vertex_init(struct processed_vertex *v, size_t varying_count) {
    v->varyings = calloc(varying_count, sizeof(mcc_vec4f));
}

void process_vertex_free(struct processed_vertex *v) {
    free(v->varyings);
}

void mcc_cpurast_render(const struct mcc_cpurast_render_config *r_config) {
    assert(r_config->r_fragment_shader->varying_count == r_config->r_vertex_shader->varying_count);
    size_t varying_count = r_config->r_vertex_shader->varying_count;
    auto depth_attachment = r_config->r_attachment->o_depth;
    auto color_attachment = r_config->r_attachment->o_color;

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
    size_t vertex_index_increment;

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
    }

    for (; vertex_idx < r_config->vertex_count; vertex_idx += vertex_index_increment) {
        switch (r_config->vertex_processing) {
        case MCC_CPURAST_VERTEX_PROCESSING_TRIANGLE_LIST:
            for (size_t di = 0; di < 3; di++) {
                vert_input.in_vertex_idx = vertex_idx + di;
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

        mcc_vec3f v0 = vertices[0].pos3,
                  v1 = vertices[1].pos3,
                  v2 = vertices[2].pos3;
        mcc_vec2f p0 = v0.xy,
                  p1 = v1.xy,
                  p2 = v2.xy;

        bool isCcw = check_point(p2, p0, p1);

        if (r_config->culling_mode == MCC_CPURAST_CULLING_MODE_CW && !isCcw)
            continue;
        if (r_config->culling_mode == MCC_CPURAST_CULLING_MODE_CCW && isCcw)
            continue;

        float wf = (float)r_config->r_attachment->width;
        float hf = (float)r_config->r_attachment->height;

        for (uint32_t y = 0; y < r_config->r_attachment->height; y++) {
            for (uint32_t x = 0; x < r_config->r_attachment->width; x++) {
                size_t idx = x + y * r_config->r_attachment->width;
                // Normalized screen coordinates
                mcc_vec2f screen_pos = {{
                    (2.f * ((float)x + 0.5f) / wf) - 1.f,
                    (2.f * ((float)y + 0.5f) / hf) - 1.f,
                }};

                auto barycentric = mcc_calculate_barycentric(p0, p1, p2, screen_pos);
                if (!barycentric.isInside)
                    continue;

                float depth = v1.z * barycentric.u + v2.z * barycentric.v + v0.z * barycentric.w;

                if (depth < 0. || depth > 1.) {
                    continue;
                }

                if (
                    depth_attachment &&
                    r_config->o_depth_comparison_fn &&
                    !r_config->o_depth_comparison_fn(depth_attachment->r_data[idx], depth)
                ) {
                    continue;
                }

                for (size_t varying_i = 0; varying_i < varying_count; varying_i++) {
                    auto varv1 = mcc_vec4f_scale(vertices[1].varyings[varying_i].vec4f, barycentric.u);
                    auto varv2 = mcc_vec4f_scale(vertices[2].varyings[varying_i].vec4f, barycentric.v);
                    auto varv0 = mcc_vec4f_scale(vertices[0].varyings[varying_i].vec4f, barycentric.w);
                    fragment_varyings[varying_i].vec4f = mcc_vec4f_add(varv0, mcc_vec4f_add(varv1, varv2));
                }

                frag_input.in_frag_coord = (mcc_vec3f){ .xy = screen_pos, .z = depth };
                r_config->r_fragment_shader->r_fn(&frag_input);

                if (depth_attachment) {
                    depth_attachment->r_data[idx] = depth;
                }

                if (color_attachment) {
                    color_attachment->r_data[idx*4+0] = (uint8_t)(frag_input.out_color.b * 255.f);
                    color_attachment->r_data[idx*4+1] = (uint8_t)(frag_input.out_color.g * 255.f);
                    color_attachment->r_data[idx*4+2] = (uint8_t)(frag_input.out_color.r * 255.f);
                    color_attachment->r_data[idx*4+3] = (uint8_t)(frag_input.out_color.a * 255.f);
                }
            }
        }
    }

    for (size_t i = 0; i < vertex_per_primitive; i++) {
        process_vertex_free(&vertices[i]);
    }
    free(fragment_varyings);
}
