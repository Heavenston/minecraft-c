#pragma once

struct mcc_color_rgb {
    float r;
    float g;
    float b;
    float a;
};

struct mcc_color_rgba {
    float r;
    float g;
    float b;
    float a;
};

struct mcc_color_rgb mcc_rgb(float r, float g, float b);

struct mcc_color_rgba mcc_rgba(float r, float g, float b, float a);
