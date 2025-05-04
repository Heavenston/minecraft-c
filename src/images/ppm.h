#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t width;
    size_t height;
    bool valid;
} mcc_image_t;

void mcc_ppm_write_file(const char *filename, const uint8_t *data, size_t width, size_t height);
void mcc_ppm_write(uint8_t *buffer, size_t buffer_size, const uint8_t *data, size_t width, size_t height, size_t *bytes_written);
mcc_image_t mcc_ppm_load_file(const char *filename);
mcc_image_t mcc_ppm_load(const uint8_t *buffer, size_t buffer_size);
