#pragma once

inline static float clampf(float val, float min, float max) {
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

inline static float min2f(float a, float b) {
    if (b < a)
        return b;
    return a;
}

inline static float min3f(float a, float b, float c) {
    return min2f(a, min2f(b, c));
}

inline static float max2f(float a, float b) {
    if (b > a)
        return b;
    return a;
}

inline static float max3f(float a, float b, float c) {
    return max2f(a, max2f(b, c));
}
