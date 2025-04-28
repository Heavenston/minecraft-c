#pragma once

#include <math.h>

typedef union mcc_vec2f {
    float components[2];
    struct {
        float x;
        float y;
    };
    struct {
        float u;
        float v;
    };
} mcc_vec2f;

typedef union mcc_vec3f {
    float components[3];
    struct {
        float x;
        float y;
        float z;
    };
    struct {
        float r;
        float g;
        float b;
    };
    mcc_vec2f xy;
    struct {
        float __padding0;
        mcc_vec2f yz;
    };
} mcc_vec3f;

typedef union mcc_vec4f {
    float components[4];
    struct {
        float x;
        float y;
        float z;
        float w;
    };
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    mcc_vec2f xy;
    mcc_vec3f xyz;
    struct {
        float __padding0;
        mcc_vec2f yz;
    };
    struct {
        float __padding1;
        mcc_vec3f yzw;
    };
    struct {
        float __padding2[2];
        mcc_vec2f zw;
    };
} mcc_vec4f;

static inline mcc_vec2f mcc_vec2f_splat(float val) {
    return (mcc_vec2f){
        .x = val,
        .y = val,
    };
}

static inline mcc_vec3f mcc_vec3f_splat(float val) {
    return (mcc_vec3f){
        .x = val,
        .y = val,
        .z = val,
    };
}

static inline mcc_vec4f mcc_vec4f_splat(float val) {
    return (mcc_vec4f){
        .x = val,
        .y = val,
        .z = val,
        .w = val,
    };
}

static inline mcc_vec2f mcc_vec2f_add(mcc_vec2f lhs, mcc_vec2f rhs) {
    return (mcc_vec2f){
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
    };
}

static inline mcc_vec3f mcc_vec3f_add(mcc_vec3f lhs, mcc_vec3f rhs) {
    return (mcc_vec3f){
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
    };
}

static inline mcc_vec4f mcc_vec4f_add(mcc_vec4f lhs, mcc_vec4f rhs) {
    return (mcc_vec4f){
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
        .w = lhs.w + rhs.w,
    };
}

static inline mcc_vec2f mcc_vec2f_sub(mcc_vec2f lhs, mcc_vec2f rhs) {
    return (mcc_vec2f){
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
    };
}

static inline mcc_vec3f mcc_vec3f_sub(mcc_vec3f lhs, mcc_vec3f rhs) {
    return (mcc_vec3f){
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
    };
}

static inline mcc_vec4f mcc_vec4f_sub(mcc_vec4f lhs, mcc_vec4f rhs) {
    return (mcc_vec4f){
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
        .w = lhs.w - rhs.w,
    };
}

static inline mcc_vec2f mcc_vec2f_mul(mcc_vec2f lhs, mcc_vec2f rhs) {
    return (mcc_vec2f){
        .x = lhs.x * rhs.x,
        .y = lhs.y * rhs.y,
    };
}

static inline mcc_vec3f mcc_vec3f_mul(mcc_vec3f lhs, mcc_vec3f rhs) {
    return (mcc_vec3f){
        .x = lhs.x * rhs.x,
        .y = lhs.y * rhs.y,
        .z = lhs.z * rhs.z,
    };
}

static inline mcc_vec4f mcc_vec4f_mul(mcc_vec4f lhs, mcc_vec4f rhs) {
    return (mcc_vec4f){
        .x = lhs.x * rhs.x,
        .y = lhs.y * rhs.y,
        .z = lhs.z * rhs.z,
        .w = lhs.w * rhs.w,
    };
}

static inline mcc_vec2f mcc_vec2f_div(mcc_vec2f lhs, mcc_vec2f rhs) {
    return (mcc_vec2f){
        .x = lhs.x / rhs.x,
        .y = lhs.y / rhs.y,
    };
}

static inline mcc_vec3f mcc_vec3f_div(mcc_vec3f lhs, mcc_vec3f rhs) {
    return (mcc_vec3f){
        .x = lhs.x / rhs.x,
        .y = lhs.y / rhs.y,
        .z = lhs.z / rhs.z,
    };
}

static inline mcc_vec4f mcc_vec4f_div(mcc_vec4f lhs, mcc_vec4f rhs) {
    return (mcc_vec4f){
        .x = lhs.x / rhs.x,
        .y = lhs.y / rhs.y,
        .z = lhs.z / rhs.z,
        .w = lhs.w / rhs.w,
    };
}

static inline mcc_vec2f mcc_vec2f_scale(mcc_vec2f lhs, float rhs) {
    return (mcc_vec2f){
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
    };
}

static inline mcc_vec3f mcc_vec3f_scale(mcc_vec3f lhs, float rhs) {
    return (mcc_vec3f){
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
    };
}
static inline mcc_vec4f mcc_vec4f_scale(mcc_vec4f lhs, float rhs) {
    return (mcc_vec4f){
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
        .w = lhs.w * rhs,
    };
}

static inline mcc_vec3f mcc_vec3f_cross(mcc_vec3f u, mcc_vec3f v) {
    return (mcc_vec3f){{
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    }};
}

static inline float mcc_vec2f_dot(mcc_vec2f lhs, mcc_vec2f rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

static inline float mcc_vec3f_dot(mcc_vec3f lhs, mcc_vec3f rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

static inline float mcc_vec4f_dot(mcc_vec4f lhs, mcc_vec4f rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

static inline float mcc_vec2f_norm(mcc_vec2f lhs) {
    return sqrtf(mcc_vec2f_dot(lhs, lhs));
}

static inline float mcc_vec3f_norm(mcc_vec3f lhs) {
    return sqrtf(mcc_vec3f_dot(lhs, lhs));
}

static inline float mcc_vec4f_norm(mcc_vec4f lhs) {
    return sqrtf(mcc_vec4f_dot(lhs, lhs));
}

static inline mcc_vec2f mcc_vec2f_normalized(mcc_vec2f lhs) {
    return mcc_vec2f_scale(lhs, 1.f / mcc_vec2f_norm(lhs));
}

static inline mcc_vec3f mcc_vec3f_normalized(mcc_vec3f lhs) {
    return mcc_vec3f_scale(lhs, 1.f / mcc_vec3f_norm(lhs));
}

static inline mcc_vec4f mcc_vec4f_normalized(mcc_vec4f lhs) {
    return mcc_vec4f_scale(lhs, 1.f / mcc_vec4f_norm(lhs));
}
