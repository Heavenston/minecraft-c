#include "transformations.h"

#include <math.h>

mcc_mat4f mcc_mat4f_rotate_x(float theta) {
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    return (mcc_mat4f) {{
        {{ 1.0f,  0.0f,   0.0f,  0.0f }},
        {{ 0.0f,  cos_t,  sin_t, 0.0f }},
        {{ 0.0f, -sin_t,  cos_t, 0.0f }},
        {{ 0.0f,  0.0f,   0.0f,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_rotate_y(float theta) {
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    return (mcc_mat4f) {{
        {{  cos_t,  0.0f, -sin_t,  0.0f }},
        {{  0.0f,   1.0f,  0.0f,  0.0f }},
        {{  sin_t,  0.0f,  cos_t, 0.0f }},
        {{  0.0f,   0.0f,  0.0f,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_rotate_z(float theta) {
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    return (mcc_mat4f) {{
        {{  cos_t,   sin_t,  0.0f,  0.0f }},
        {{ -sin_t,   cos_t,  0.0f,  0.0f }},
        {{  0.0f,    0.0f,  1.0f,  0.0f }},
        {{  0.0f,    0.0f,  0.0f,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_translate(mcc_vec3f delta) {
    return (mcc_mat4f) {{
        {{ 1.0f,    0.0f,    0.0f,     0.0f }},
        {{ 0.0f,    1.0f,    0.0f,     0.0f }},
        {{ 0.0f,    0.0f,    1.0f,     0.0f }},
        {{ delta.x, delta.y, delta.z,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_translate_x(float delta) {
    return (mcc_mat4f) {{
        {{ 1.0f,  0.0f,  0.0f,  0.0f }},
        {{ 0.0f,  1.0f,  0.0f,  0.0f }},
        {{ 0.0f,  0.0f,  1.0f,  0.0f }},
        {{ delta, 0.0f,  0.0f,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_translate_y(float delta) {
    return (mcc_mat4f) {{
        {{ 1.0f,  0.0f,  0.0f,  0.0f }},
        {{ 0.0f,  1.0f,  0.0f,  0.0f }},
        {{ 0.0f,  0.0f,  1.0f,  0.0f }},
        {{ 0.0f,  delta, 0.0f,  1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_translate_z(float delta) {
    return (mcc_mat4f) {{
        {{ 1.0f,  0.0f,  0.0f,  0.0f }},
        {{ 0.0f,  1.0f,  0.0f,  0.0f }},
        {{ 0.0f,  0.0f,  1.0f,  0.0f }},
        {{ 0.0f,  0.0f,  delta, 1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_x(float factor) {
    return (mcc_mat4f) {{
        {{ factor, 0.0f,   0.0f,   0.0f }},
        {{ 0.0f,   1.0f,   0.0f,   0.0f }},
        {{ 0.0f,   0.0f,   1.0f,   0.0f }},
        {{ 0.0f,   0.0f,   0.0f,   1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_y(float factor) {
    return (mcc_mat4f) {{
        {{ 1.0f,   0.0f,   0.0f,   0.0f }},
        {{ 0.0f,   factor, 0.0f,   0.0f }},
        {{ 0.0f,   0.0f,   1.0f,   0.0f }},
        {{ 0.0f,   0.0f,   0.0f,   1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_z(float factor) {
    return (mcc_mat4f) {{
        {{ 1.0f,   0.0f,   0.0f,   0.0f }},
        {{ 0.0f,   1.0f,   0.0f,   0.0f }},
        {{ 0.0f,   0.0f,   factor, 0.0f }},
        {{ 0.0f,   0.0f,   0.0f,   1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_xy(float factor_x, float factor_y) {
    return (mcc_mat4f) {{
        {{ factor_x, 0.0f,     0.0f,   0.0f }},
        {{ 0.0f,     factor_y, 0.0f,   0.0f }},
        {{ 0.0f,     0.0f,     1.0f,   0.0f }},
        {{ 0.0f,     0.0f,     0.0f,   1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_xz(float factor_x, float factor_z) {
    return (mcc_mat4f) {{
        {{ factor_x, 0.0f,   0.0f,     0.0f }},
        {{ 0.0f,     1.0f,   0.0f,     0.0f }},
        {{ 0.0f,     0.0f,   factor_z, 0.0f }},
        {{ 0.0f,     0.0f,   0.0f,     1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_yz(float factor_y, float factor_z) {
    return (mcc_mat4f) {{
        {{ 1.0f,   0.0f,     0.0f,     0.0f }},
        {{ 0.0f,   factor_y, 0.0f,     0.0f }},
        {{ 0.0f,   0.0f,     factor_z, 0.0f }},
        {{ 0.0f,   0.0f,     0.0f,     1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_scale_xyz(float factor_x, float factor_y, float factor_z) {
    return (mcc_mat4f) {{
        {{ factor_x, 0.0f,     0.0f,     0.0f }},
        {{ 0.0f,     factor_y, 0.0f,     0.0f }},
        {{ 0.0f,     0.0f,     factor_z, 0.0f }},
        {{ 0.0f,     0.0f,     0.0f,     1.0f }},
    }};
}

mcc_mat4f mcc_mat4f_perspective(float aspect_ratio, float z_near, float z_far, float fov_rad) {
    float tan_half_fov = tanf(fov_rad / 2.0f);
    float z_range = z_far - z_near;
    float f1 = 1 / (aspect_ratio * tan_half_fov);
    float f2 = 1 / tan_half_fov;
    float f3 = (z_far + z_near) / z_range;
    float f4 = -2.0f * z_near * z_far / z_range;

    return (mcc_mat4f) {{
        {{ f1,    0.0f,  0.0f,  0.0f }},
        {{ 0.0f,  f2,    0.0f,  0.0f }},
        {{ 0.0f,  0.0f,  f3,    1.0f }},
        {{ 0.0f,  0.0f,  f4,    0.0f }},
    }};
}
