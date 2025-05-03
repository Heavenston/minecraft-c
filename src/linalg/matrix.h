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

typedef union mcc_mat4f {
    struct {
        mcc_vec4f r1;
        mcc_vec4f r2;
        mcc_vec4f r3;
        mcc_vec4f r4;
    };
    float comps[4][4];
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
    return (mcc_mat4f) {{
        {{ val, val, val, val }},
        {{ val, val, val, val }},
        {{ val, val, val, val }},
        {{ val, val, val, val }},
    }};
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
    return (mcc_mat4f) {{
        {{ mat.r1.x, mat.r2.x, mat.r3.x, mat.r4.x }},
        {{ mat.r1.y, mat.r2.y, mat.r3.y, mat.r4.y }},
        {{ mat.r1.z, mat.r2.z, mat.r3.z, mat.r4.z }},
        {{ mat.r1.w, mat.r2.w, mat.r3.w, mat.r4.w }},
    }};
}

static inline mcc_mat2f mcc_mat2f_col(mcc_vec2f col1, mcc_vec2f col2) {
    return mcc_mat2f_transpose((mcc_mat2f){ col1, col2 });
}

static inline mcc_mat3f mcc_mat3f_col(mcc_vec3f col1, mcc_vec3f col2, mcc_vec3f col3) {
    return mcc_mat3f_transpose((mcc_mat3f) { col1, col2, col3 });
}

static inline mcc_mat4f mcc_mat4f_col(mcc_vec4f col1, mcc_vec4f col2, mcc_vec4f col3, mcc_vec4f col4) {
    return mcc_mat4f_transpose((mcc_mat4f) {{ col1, col2, col3, col4 }});
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

static inline mcc_mat4f mcc_mat4f_identity() {
    return (mcc_mat4f) {{
        {{ 1.f, 0.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 0.f, 1.f }},
    }};
}

static inline mcc_mat2f mcc_mat2f_mul(mcc_mat2f a, mcc_mat2f b) {
    mcc_mat2f result;
    
    result.r1.x = a.r1.x * b.r1.x + a.r1.y * b.r2.x;
    result.r1.y = a.r1.x * b.r1.y + a.r1.y * b.r2.y;
    
    result.r2.x = a.r2.x * b.r1.x + a.r2.y * b.r2.x;
    result.r2.y = a.r2.x * b.r1.y + a.r2.y * b.r2.y;
    
    return result;
}

static inline mcc_mat3f mcc_mat3f_mul(mcc_mat3f a, mcc_mat3f b) {
    mcc_mat3f result;
    
    result.r1.x = a.r1.x * b.r1.x + a.r1.y * b.r2.x + a.r1.z * b.r3.x;
    result.r1.y = a.r1.x * b.r1.y + a.r1.y * b.r2.y + a.r1.z * b.r3.y;
    result.r1.z = a.r1.x * b.r1.z + a.r1.y * b.r2.z + a.r1.z * b.r3.z;
    
    result.r2.x = a.r2.x * b.r1.x + a.r2.y * b.r2.x + a.r2.z * b.r3.x;
    result.r2.y = a.r2.x * b.r1.y + a.r2.y * b.r2.y + a.r2.z * b.r3.y;
    result.r2.z = a.r2.x * b.r1.z + a.r2.y * b.r2.z + a.r2.z * b.r3.z;
    
    result.r3.x = a.r3.x * b.r1.x + a.r3.y * b.r2.x + a.r3.z * b.r3.x;
    result.r3.y = a.r3.x * b.r1.y + a.r3.y * b.r2.y + a.r3.z * b.r3.y;
    result.r3.z = a.r3.x * b.r1.z + a.r3.y * b.r2.z + a.r3.z * b.r3.z;
    
    return result;
}

static inline mcc_mat4f mcc_mat4f_mul(mcc_mat4f a, mcc_mat4f b) {
    mcc_mat4f result = {};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.comps[j][i] += a.comps[k][i] * b.comps[j][k];
            }
        }
    }
    
    return result;
}

static inline float mcc_mat2f_det(mcc_mat2f mat) {
    return mat.r1.x * mat.r2.y - mat.r1.y * mat.r2.x;
}

static inline float mcc_mat3f_det(mcc_mat3f mat) {
    float a = mat.r1.x * (mat.r2.y * mat.r3.z - mat.r2.z * mat.r3.y);
    float b = mat.r1.y * (mat.r2.x * mat.r3.z - mat.r2.z * mat.r3.x);
    float c = mat.r1.z * (mat.r2.x * mat.r3.y - mat.r2.y * mat.r3.x);
    return a - b + c;
}

static inline float mcc_mat4f_det(mcc_mat4f mat) {
    // Calculate the determinant using cofactor expansion along the first row
    float det00 = mat.r2.y * (mat.r3.z * mat.r4.w - mat.r3.w * mat.r4.z) -
                  mat.r2.z * (mat.r3.y * mat.r4.w - mat.r3.w * mat.r4.y) +
                  mat.r2.w * (mat.r3.y * mat.r4.z - mat.r3.z * mat.r4.y);
    
    float det01 = mat.r2.x * (mat.r3.z * mat.r4.w - mat.r3.w * mat.r4.z) -
                  mat.r2.z * (mat.r3.x * mat.r4.w - mat.r3.w * mat.r4.x) +
                  mat.r2.w * (mat.r3.x * mat.r4.z - mat.r3.z * mat.r4.x);
    
    float det02 = mat.r2.x * (mat.r3.y * mat.r4.w - mat.r3.w * mat.r4.y) -
                  mat.r2.y * (mat.r3.x * mat.r4.w - mat.r3.w * mat.r4.x) +
                  mat.r2.w * (mat.r3.x * mat.r4.y - mat.r3.y * mat.r4.x);
    
    float det03 = mat.r2.x * (mat.r3.y * mat.r4.z - mat.r3.z * mat.r4.y) -
                  mat.r2.y * (mat.r3.x * mat.r4.z - mat.r3.z * mat.r4.x) +
                  mat.r2.z * (mat.r3.x * mat.r4.y - mat.r3.y * mat.r4.x);
    
    return mat.r1.x * det00 - mat.r1.y * det01 + mat.r1.z * det02 - mat.r1.w * det03;
}

static inline mcc_vec2f mcc_mat2f_mul_vec2f(mcc_mat2f mat, mcc_vec2f vec) {
    return (mcc_vec2f){
        .x = mat.r1.x * vec.x + mat.r1.y * vec.y,
        .y = mat.r2.x * vec.x + mat.r2.y * vec.y,
    };
}

static inline mcc_vec3f mcc_mat3f_mul_vec3f(mcc_mat3f mat, mcc_vec3f vec) {
    return (mcc_vec3f){
        .x = mat.r1.x * vec.x + mat.r1.y * vec.y + mat.r1.z * vec.z,
        .y = mat.r2.x * vec.x + mat.r2.y * vec.y + mat.r2.z * vec.z,
        .z = mat.r3.x * vec.x + mat.r3.y * vec.y + mat.r3.z * vec.z,
    };
}

static inline mcc_vec4f mcc_mat4f_mul_vec4f(mcc_mat4f mat, mcc_vec4f vec) {
    mcc_vec4f out;
    for (int i = 0; i < 4; ++i) {
        out.components[i] =
              mat.comps[0][i] * vec.components[0]
            + mat.comps[1][i] * vec.components[1]
            + mat.comps[2][i] * vec.components[2]
            + mat.comps[3][i] * vec.components[3];
    }
    return out;
}
