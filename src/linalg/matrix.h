#pragma once

#include "linalg/vector.h"

typedef struct mcc_mat2f {
    mcc_vec2f r1;
    mcc_vec2f r2;
} mcc_mat2f;

typedef struct mcc_mat3f {
    mcc_vec3f r1;
    mcc_vec3f r2;
    mcc_vec3f r3;
} mcc_mat3f;

typedef struct mcc_mat4f {
    mcc_vec4f r1;
    mcc_vec4f r2;
    mcc_vec4f r3;
    mcc_vec4f r4;
} mcc_mat4f;

static inline mcc_mat2f mcc_mat2f_splat(float val) {
    return (mcc_mat2f) {
        {{ val, val }},
        {{ val, val }},
    };
}

static inline mcc_mat3f mcc_mat3f_splat(float val) {
    return (mcc_mat3f) {
        {{ val, val, val }},
        {{ val, val, val }},
        {{ val, val, val }},
    };
}

static inline mcc_mat4f mcc_mat4f_splat(float val) {
    return (mcc_mat4f) {
        {{ val, val, val, val }},
        {{ val, val, val, val }},
        {{ val, val, val, val }},
        {{ val, val, val, val }},
    };
}

static inline mcc_mat2f mcc_mat2f_transpose(mcc_mat2f mat) {
    return (mcc_mat2f) {
        {{ mat.r1.x, mat.r2.x }},
        {{ mat.r1.y, mat.r2.y }},
    };
}

static inline mcc_mat3f mcc_mat3f_transpose(mcc_mat3f mat) {
    return (mcc_mat3f) {
        {{ mat.r1.x, mat.r2.x, mat.r3.x }},
        {{ mat.r1.y, mat.r2.y, mat.r3.y }},
        {{ mat.r1.z, mat.r2.z, mat.r3.z }},
    };
}

static inline mcc_mat4f mcc_mat4f_transpose(mcc_mat4f mat) {
    return (mcc_mat4f) {
        {{ mat.r1.x, mat.r2.x, mat.r3.x, mat.r4.x }},
        {{ mat.r1.y, mat.r2.y, mat.r3.y, mat.r4.y }},
        {{ mat.r1.z, mat.r2.z, mat.r3.z, mat.r4.z }},
        {{ mat.r1.w, mat.r2.w, mat.r3.w, mat.r4.w }},
    };
}

static inline mcc_mat2f mcc_mat2f_col(mcc_vec2f col1, mcc_vec2f col2) {
    return mcc_mat2f_transpose((mcc_mat2f){ col1, col2 });
}

static inline mcc_mat3f mcc_mat3f_col(mcc_vec3f col1, mcc_vec3f col2, mcc_vec3f col3) {
    return mcc_mat3f_transpose((mcc_mat3f) { col1, col2, col3 });
}

static inline mcc_mat4f mcc_mat4f_col(mcc_vec4f col1, mcc_vec4f col2, mcc_vec4f col3, mcc_vec4f col4) {
    return mcc_mat4f_transpose((mcc_mat4f) { col1, col2, col3, col4 });
}

static inline mcc_mat2f mcc_mat2f_ident() {
    return (mcc_mat2f) {
        {{ 1.f, 0.f }},
        {{ 0.f, 1.f }},
    };
}

static inline mcc_mat3f mcc_mat3f_ident() {
    return (mcc_mat3f) {
        {{ 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f }},
    };
}

static inline mcc_mat4f mcc_mat4f_ident() {
    return (mcc_mat4f) {
        {{ 1.f, 0.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 0.f, 1.f }},
    };
}

static inline mcc_mat2f mcc_mat2f_mul(mcc_mat2f, mcc_mat2f) {
    // TODO
    return mcc_mat2f_ident();
}

static inline mcc_mat3f mcc_mat3f_mul(mcc_mat3f, mcc_mat3f) {
    // TODO
    return mcc_mat3f_ident();
}

static inline mcc_mat4f mcc_mat4f_mul(mcc_mat4f, mcc_mat4f) {
    // TODO
    return mcc_mat4f_ident();
}

static inline float mcc_mat2f_det(mcc_mat2f mat) {
    return mat.r1.x * mat.r2.y - mat.r1.y * mat.r2.x;
}
