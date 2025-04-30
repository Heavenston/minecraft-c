#pragma once

#include <stdint.h>
#include <stddef.h>

void mcc_ppm_write(const char *filename, const uint8_t *data, size_t width, size_t height);
