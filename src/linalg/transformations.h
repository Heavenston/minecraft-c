#pragma once

#include "matrix.h"

mcc_mat4f mcc_mat4f_rotate_x(float theta);

mcc_mat4f mcc_mat4f_rotate_y(float theta);

mcc_mat4f mcc_mat4f_rotate_z(float theta);

mcc_mat4f mcc_mat4f_translate(mcc_vec3f delta);

mcc_mat4f mcc_mat4f_translate_x(float theta);

mcc_mat4f mcc_mat4f_translate_y(float theta);

mcc_mat4f mcc_mat4f_translate_z(float theta);

mcc_mat4f mcc_mat4f_scale_x(float factor);

mcc_mat4f mcc_mat4f_scale_y(float factor);

mcc_mat4f mcc_mat4f_scale_z(float factor);

mcc_mat4f mcc_mat4f_scale_xy(float factor_x, float factor_y);

mcc_mat4f mcc_mat4f_scale_xz(float factor_x, float factor_z);

mcc_mat4f mcc_mat4f_scale_yz(float factor_y, float factor_z);

mcc_mat4f mcc_mat4f_scale_xyz(float factor_x, float factor_y, float factor_z);

mcc_mat4f mcc_mat4f_perspective(float aspect_ratio, float z_near, float z_far, float fov_rad);
