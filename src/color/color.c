#include "color.h"

struct mcc_color_rgb mcc_rgb(float r, float g, float b) {
    return (struct mcc_color_rgb) {
        .r = r,
        .g = g,
        .b = b,
    };
}

struct mcc_color_rgba mcc_rgba(float r, float g, float b, float a) {
    return (struct mcc_color_rgba) {
        .r = r,
        .g = g,
        .b = b,
        .a = a,
    };
}
