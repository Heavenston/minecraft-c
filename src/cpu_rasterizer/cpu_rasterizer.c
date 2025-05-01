#include "cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/vector.h"

#include <assert.h>
#include <stdlib.h>

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

void mcc_cpurast_render(const struct mcc_cpurast_render_config *r_config) {
    assert(r_config->r_fragment_shader->varying_count == r_config->r_vertex_shader->varying_count);
    size_t varying_count = r_config->r_vertex_shader->varying_count;
    auto depth_attachment = r_config->r_attachment->o_depth;
    auto color_attachment = r_config->r_attachment->o_color;

    union mcc_cpurast_shaders_varying *varyings_v0 = calloc(varying_count, sizeof(mcc_vec4f));
    union mcc_cpurast_shaders_varying *varyings_v1 = calloc(varying_count, sizeof(mcc_vec4f));
    union mcc_cpurast_shaders_varying *varyings_v2 = calloc(varying_count, sizeof(mcc_vec4f));
    union mcc_cpurast_shaders_varying *fragment_varyings = calloc(varying_count, sizeof(mcc_vec4f));

    struct mcc_cpurast_fragment_shader_input frag_input = {
        .o_in_data = r_config->o_fragment_shader_data,
        .r_in_varyings = fragment_varyings,
    };
    struct mcc_cpurast_vertex_shader_input vert_input = {
        .o_in_data = r_config->o_vertex_shader_data,
    };

    for (uint32_t vi = 0; vi < r_config->vertex_count; vi += 3) {
        vert_input.in_vertex_idx = vi + 0;
        vert_input.r_out_varyings = varyings_v0;
        r_config->r_vertex_shader->r_fn(&vert_input);
        mcc_vec3f v0 = mcc_vec3f_scale(vert_input.out_position.xyz, 1.f / vert_input.out_position.w);

        vert_input.in_vertex_idx = vi + 1;
        vert_input.r_out_varyings = varyings_v1;
        r_config->r_vertex_shader->r_fn(&vert_input);
        mcc_vec3f v1 = mcc_vec3f_scale(vert_input.out_position.xyz, 1.f / vert_input.out_position.w);

        vert_input.in_vertex_idx = vi + 2;
        vert_input.r_out_varyings = varyings_v2;
        r_config->r_vertex_shader->r_fn(&vert_input);
        mcc_vec3f v2 = mcc_vec3f_scale(vert_input.out_position.xyz, 1.f / vert_input.out_position.w);

        bool isCcw = check_point(v2.xy, v0.xy, v1.xy);

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

                auto barycentric = mcc_calculate_barycentric(v0.xy, v1.xy, v2.xy, screen_pos);
                if (!barycentric.isInside)
                    continue;

                float depth = v1.z * barycentric.u + v2.z * barycentric.v + v0.z * barycentric.w;

                // TODO: Clipping
                // if (depth < 0. || depth > 1.)
                //     continue;

                if (
                    depth_attachment &&
                    r_config->o_depth_comparison_fn &&
                    !r_config->o_depth_comparison_fn(depth_attachment->r_data[idx], depth)
                ) {
                    continue;
                }

                for (size_t varying_i = 0; varying_i < varying_count; varying_i++) {
                    auto varv0 = mcc_vec4f_scale(varyings_v1[varying_i].vec4f, barycentric.u);
                    auto varv1 = mcc_vec4f_scale(varyings_v2[varying_i].vec4f, barycentric.v);
                    auto varv2 = mcc_vec4f_scale(varyings_v0[varying_i].vec4f, barycentric.w);
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

    free(varyings_v0);
    free(varyings_v1);
    free(varyings_v2);
    free(fragment_varyings);
}
