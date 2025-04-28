#pragma once

#include "matrix.h"

mcc_mat4f mcc_mat4f_rotate_x(float theta);

mcc_mat4f mcc_mat4f_rotate_y(float theta);

mcc_mat4f mcc_mat4f_rotate_z(float theta);

mcc_mat4f mcc_mat4f_translate_x(float theta);

mcc_mat4f mcc_mat4f_translate_y(float theta);

mcc_mat4f mcc_mat4f_translate_z(float theta);

mcc_mat4f mcc_mat4f_perspective(float ar, float z_near, float z_far, float alpha);
