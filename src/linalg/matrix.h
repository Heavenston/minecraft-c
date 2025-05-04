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

static inline mcc_mat4f mcc_mat4f_inverse(mcc_mat4f m) {
    float aug[4][8];
    int i, j, k;

    /* Build augmented matrix [m | I] */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            aug[i][j]     = m.comps[i][j];
            aug[i][j + 4] = (i == j) ? 1.0f : 0.0f;
        }
    }

    /* Forward elimination */
    for (i = 0; i < 4; i++) {
        /* Find pivot row */
        int   pivot = i;
        float max   = fabsf(aug[i][i]);
        for (k = i + 1; k < 4; k++) {
            float v = fabsf(aug[k][i]);
            if (v > max) {
                max   = v;
                pivot = k;
            }
        }
        /* Singular? return identity */
        if (max < 1e-6f) {
            return (mcc_mat4f){ .comps = {
                {1,0,0,0},
                {0,1,0,0},
                {0,0,1,0},
                {0,0,0,1}
            }};
        }
        /* Swap rows if needed */
        if (pivot != i) {
            for (j = 0; j < 8; j++) {
                float tmp     = aug[i][j];
                aug[i][j]     = aug[pivot][j];
                aug[pivot][j] = tmp;
            }
        }
        /* Normalize pivot row */
        float diag = aug[i][i];
        for (j = 0; j < 8; j++) {
            aug[i][j] /= diag;
        }
        /* Eliminate other rows */
        for (k = 0; k < 4; k++) {
            if (k == i) continue;
            float factor = aug[k][i];
            for (j = 0; j < 8; j++) {
                aug[k][j] -= factor * aug[i][j];
            }
        }
    }

    /* Extract inverse from augmented matrix */
    mcc_mat4f inv;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            inv.comps[i][j] = aug[i][j + 4];
        }
    }
    return inv;
}
