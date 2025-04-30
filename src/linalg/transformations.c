#include "transformations.h"

#include <math.h>

mcc_mat4f mcc_mat4f_rotate_x(float theta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[1][1] = cosf(theta);
    mat.comps[2][1] = -sinf(theta);
    mat.comps[1][2] = sinf(theta);
    mat.comps[2][2] = cosf(theta);
    return mat;
}

mcc_mat4f mcc_mat4f_rotate_y(float theta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[2][2] = cosf(theta);
    mat.comps[0][2] = -sinf(theta);
    mat.comps[2][0] = sinf(theta);
    mat.comps[0][0] = cosf(theta);
    return mat;
}

mcc_mat4f mcc_mat4f_rotate_z(float theta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[0][0] = cosf(theta);
    mat.comps[1][0] = -sinf(theta);
    mat.comps[0][1] = sinf(theta);
    mat.comps[1][1] = cosf(theta);
    return mat;
}

mcc_mat4f mcc_mat4f_translate_x(float delta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[3][0] = delta;
    return mat;
}

mcc_mat4f mcc_mat4f_translate_y(float delta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[3][1] = delta;
    return mat;
}

mcc_mat4f mcc_mat4f_translate_z(float delta) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[3][2] = delta;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_x(float factor) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[0][0] = factor;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_y(float factor) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[1][1] = factor;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_z(float factor) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[2][2] = factor;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_xy(float factor_x, float factor_y) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[0][0] = factor_x;
    mat.comps[1][1] = factor_y;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_xz(float factor_x, float factor_z) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[0][0] = factor_x;
    mat.comps[2][2] = factor_z;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_yz(float factor_y, float factor_z) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[1][1] = factor_y;
    mat.comps[2][2] = factor_z;
    return mat;
}

mcc_mat4f mcc_mat4f_scale_xyz(float factor_x, float factor_y, float factor_z) {
    mcc_mat4f mat = mcc_mat4f_identity();
    mat.comps[0][0] = factor_x;
    mat.comps[1][1] = factor_y;
    mat.comps[2][2] = factor_z;
    return mat;
}

mcc_mat4f mcc_mat4f_perspective(float ar, float z_near, float z_far, float alpha) {
    float tan_half_alpha = tanf(alpha / 2.0f);
    float z_range = z_far - z_near;

    mcc_mat4f mat = {};
    mat.comps[0][0] = 1 / (ar * tan_half_alpha);
    mat.comps[1][1] = 1 / tan_half_alpha;
    mat.comps[2][2] = (z_far + z_near) / z_range;
    mat.comps[3][2] = -2.0f * z_near * z_far / z_range;
    mat.comps[2][3] = 1.0f;
    return mat;
}
