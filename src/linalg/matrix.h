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

static inline mcc_vec4f mcc_mat4f_mul_vec4f(mcc_mat4f mat, mcc_vec4f vec) {
    mcc_mat4f t = mcc_mat4f_transpose(mat);
    return (mcc_vec4f) {
        .x = mcc_vec4f_dot(t.r1, vec),
        .y = mcc_vec4f_dot(t.r2, vec),
        .z = mcc_vec4f_dot(t.r3, vec),
        .w = mcc_vec4f_dot(t.r4, vec),
    };
}

static inline mcc_mat4f mcc_mat4f_inverse(mcc_mat4f mat) {
    mcc_vec4f c0 = {{ mat.r1.x, mat.r2.x, mat.r3.x, mat.r4.x }};
    mcc_vec4f c1 = {{ mat.r1.y, mat.r2.y, mat.r3.y, mat.r4.y }};
    mcc_vec4f c2 = {{ mat.r1.z, mat.r2.z, mat.r3.z, mat.r4.z }};
    mcc_vec4f c3 = {{ mat.r1.w, mat.r2.w, mat.r3.w, mat.r4.w }};

    mcc_vec3f a0 = {{ c2.y, c2.z, c2.w }};
    mcc_vec3f a1 = {{ c3.y, c3.z, c3.w }};
    mcc_vec3f a2 = {{ c1.y, c1.z, c1.w }};
    mcc_vec3f a3 = {{ c3.y, c3.z, c3.w }};
    mcc_vec3f a4 = {{ c1.y, c1.z, c1.w }};
    mcc_vec3f a5 = {{ c2.y, c2.z, c2.w }};

    mcc_vec3f c20_c31 = mcc_vec3f_cross(a0, a1);
    mcc_vec3f c10_c32 = mcc_vec3f_cross(a2, a3);
    mcc_vec3f c10_c21 = mcc_vec3f_cross(a4, a5);

    mcc_vec3f b0 = {{ c2.x, c2.z, c2.w }};
    mcc_vec3f b1 = {{ c3.x, c3.z, c3.w }};
    mcc_vec3f b2 = {{ c1.x, c1.z, c1.w }};
    mcc_vec3f b3 = {{ c3.x, c3.z, c3.w }};
    mcc_vec3f b4 = {{ c1.x, c1.z, c1.w }};
    mcc_vec3f b5 = {{ c2.x, c2.z, c2.w }};

    mcc_vec3f c20_c30 = mcc_vec3f_cross(b0, b1);
    mcc_vec3f c10_c30 = mcc_vec3f_cross(b2, b3);
    mcc_vec3f c10_c20 = mcc_vec3f_cross(b4, b5);

    float d01 = c1.x * c3.w - c1.w * c3.x;
    float d04 = c1.y * c3.z - c1.z * c3.y;
    float d05 = c1.y * c3.w - c1.w * c3.y;

    float d11 = c0.x * c3.w - c0.w * c3.x;
    float d14 = c0.y * c3.z - c0.z * c3.y;
    float d15 = c0.y * c3.w - c0.w * c3.y;

    float d21 = c0.x * c2.w - c0.w * c2.x;
    float d24 = c0.y * c2.z - c0.z * c2.y;
    float d25 = c0.y * c2.w - c0.w * c2.y;

    float d31 = c0.x * c1.w - c0.w * c1.x;
    float d34 = c0.y * c1.z - c0.z * c1.y;
    float d35 = c0.y * c1.w - c0.w * c1.y;

    mcc_vec4f v0, v1, v2, v3;

    v0.x =  c20_c31.z;
    v0.y = -c10_c32.z;
    v0.z =  c10_c21.z;
    v0.w = -(c2.x * d05 - c2.y * d01 + c2.w * d04);

    v1.x = -c20_c30.z;
    v1.y =  c10_c30.z;
    v1.z = -c10_c20.z;
    v1.w =  (c3.x * d25 - c3.y * d21 + c3.w * d24);

    v2.x =  c20_c30.y;
    v2.y = -c10_c30.y;
    v2.z =  c10_c20.y;
    v2.w = -(c3.x * d15 - c3.y * d11 + c3.w * d14);

    v3.x = -c20_c30.x;
    v3.y =  c10_c30.x;
    v3.z = -c10_c20.x;
    v3.w =  (c2.x * d35 - c2.y * d31 + c2.w * d34);

    float det = c0.x * v0.x + c0.y * v1.x + c0.z * v2.x + c0.w * v3.x;
    float inv_det = 1.0f / det;

    mcc_mat4f result;
    result.r1 = mcc_vec4f_scale(v0, inv_det);
    result.r2 = mcc_vec4f_scale(v1, inv_det);
    result.r3 = mcc_vec4f_scale(v2, inv_det);
    result.r4 = mcc_vec4f_scale(v3, inv_det);

    return result;
}
