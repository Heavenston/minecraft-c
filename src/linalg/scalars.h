#pragma once

inline static float clampf(float val, float min, float max) {
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}
