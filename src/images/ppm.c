#include "ppm.h"
#include <stdio.h>

void mcc_ppm_write(const char *filename, const uint8_t *data, size_t width, size_t height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }
    
    // Write PPM header (P6 format - binary RGB)
    fprintf(fp, "P6\n%zu %zu\n255\n", width, height);
    
    // Write pixel data (convert BGRA to RGB)
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 4; // BGRA format (4 bytes per pixel)
            uint8_t b = data[idx];
            uint8_t g = data[idx + 1];
            uint8_t r = data[idx + 2];
            
            // Write RGB (PPM expects RGB order)
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }
    
    fclose(fp);
}
