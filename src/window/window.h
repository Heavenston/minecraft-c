#pragma once

#include <stdint.h>

struct mcc_window;

struct mcc_create_window_cfg {
    const char *title;
    uint16_t width;
    uint16_t height;
    uint16_t min_width;
    uint16_t min_height;
};

struct mcc_window *mcc_window_create(struct mcc_create_window_cfg);

void mcc_window_open(struct mcc_window *);
void mcc_window_close(struct mcc_window *);

void mcc_window_free(struct mcc_window *);
