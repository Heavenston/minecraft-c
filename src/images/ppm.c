#include "ppm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mcc_ppm_write_file(const char *filename, const uint8_t *data, size_t width, size_t height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }
    
    fprintf(fp, "P6\n%zu %zu\n255\n", width, height);
    
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 4;
            uint8_t b = data[idx];
            uint8_t g = data[idx + 1];
            uint8_t r = data[idx + 2];
            
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }
    
    fclose(fp);
}

void mcc_ppm_write(uint8_t *buffer, size_t buffer_size, const uint8_t *data, size_t width, size_t height, size_t *bytes_written) {
    if (!buffer || buffer_size == 0) {
        if (bytes_written) *bytes_written = 0;
        return;
    }
    
    char header[50];
    int len = snprintf(header, sizeof(header), "P6\n%zu %zu\n255\n", width, height);
    if (len < 0) {
        fprintf(stderr, "Error formatting PPM header\n");
        if (bytes_written) *bytes_written = 0;
        return;
    }
    size_t header_len = (size_t)len;
    
    size_t required_size = header_len + (width * height * 3);
    
    if (buffer_size < required_size) {
        fprintf(stderr, "Buffer too small for image data. Need %zu bytes\n", required_size);
        if (bytes_written) *bytes_written = 0;
        return;
    }
    
    memcpy(buffer, header, header_len);
    size_t offset = header_len;
    
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 4;
            uint8_t b = data[idx];
            uint8_t g = data[idx + 1];
            uint8_t r = data[idx + 2];
            
            buffer[offset++] = r;
            buffer[offset++] = g;
            buffer[offset++] = b;
        }
    }
    
    if (bytes_written) *bytes_written = offset;
}

mcc_image_t mcc_ppm_load_file(const char *filename) {
    mcc_image_t image = {NULL, 0, 0, false};
    
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s for reading\n", filename);
        return image;
    }
    
    char magic[3] = {0};
    if (fscanf(fp, "%2s\n", magic) != 1 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "Invalid PPM file format (not P6)\n");
        fclose(fp);
        return image;
    }
    
    // Skip comments
    int c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n');
        c = getc(fp);
    }
    ungetc(c, fp);
    
    if (fscanf(fp, "%zu %zu\n", &image.width, &image.height) != 2) {
        fprintf(stderr, "Failed to read image dimensions\n");
        fclose(fp);
        return image;
    }
    
    int max_val;
    if (fscanf(fp, "%d\n", &max_val) != 1 || max_val != 255) {
        fprintf(stderr, "Unsupported color depth (only 8-bit per channel supported)\n");
        fclose(fp);
        return image;
    }
    
    size_t size = image.width * image.height * 4;
    image.data = (uint8_t*)calloc(1, size);
    if (!image.data) {
        fprintf(stderr, "Failed to allocate memory for image data\n");
        fclose(fp);
        return image;
    }
    
    for (size_t y = 0; y < image.height; y++) {
        for (size_t x = 0; x < image.width; x++) {
            size_t idx = (y * image.width + x) * 4;
            uint8_t r, g, b;
            if (fread(&r, 1, 1, fp) != 1 ||
                fread(&g, 1, 1, fp) != 1 ||
                fread(&b, 1, 1, fp) != 1) {
                fprintf(stderr, "Failed to read pixel data\n");
                free(image.data);
                image.data = NULL;
                fclose(fp);
                return image;
            }
            
            image.data[idx] = b;
            image.data[idx + 1] = g;
            image.data[idx + 2] = r;
            image.data[idx + 3] = 255;
        }
    }
    
    fclose(fp);
    image.valid = true;
    return image;
}

mcc_image_t mcc_ppm_load(const uint8_t *buffer, size_t buffer_size) {
    mcc_image_t image = {NULL, 0, 0, false};
    
    if (!buffer || buffer_size == 0) {
        fprintf(stderr, "Invalid buffer for PPM loading\n");
        return image;
    }
    
    size_t position = 0;
    if (buffer_size < 3 || buffer[0] != 'P' || buffer[1] != '6' || buffer[2] != '\n') {
        fprintf(stderr, "Invalid PPM format (not P6)\n");
        return image;
    }
    position = 3;
    
    // Skip comments
    while (position < buffer_size && buffer[position] == '#') {
        while (position < buffer_size && buffer[position] != '\n') {
            position++;
        }
        position++;
    }
    
    size_t width = 0;
    while (position < buffer_size && buffer[position] >= '0' && buffer[position] <= '9') {
        width = width * 10 + (buffer[position] - '0');
        position++;
    }
    
    while (position < buffer_size && (buffer[position] == ' ' || buffer[position] == '\t')) {
        position++;
    }
    
    size_t height = 0;
    while (position < buffer_size && buffer[position] >= '0' && buffer[position] <= '9') {
        height = height * 10 + (buffer[position] - '0');
        position++;
    }
    
    while (position < buffer_size && buffer[position] != '\n') {
        position++;
    }
    position++;
    
    int max_val = 0;
    while (position < buffer_size && buffer[position] >= '0' && buffer[position] <= '9') {
        max_val = max_val * 10 + (buffer[position] - '0');
        position++;
    }
    
    while (position < buffer_size && buffer[position] != '\n') {
        position++;
    }
    position++;
    
    if (max_val != 255) {
        fprintf(stderr, "Unsupported color depth (only 8-bit per channel supported)\n");
        return image;
    }
    
    if (width == 0 || height == 0) {
        fprintf(stderr, "Invalid image dimensions: %zux%zu\n", width, height);
        return image;
    }
    
    if (buffer_size < position + width * height * 3) {
        fprintf(stderr, "Buffer too small for image data\n");
        return image;
    }
    
    image.width = width;
    image.height = height;
    
    size_t size = width * height * 4;
    image.data = (uint8_t*)calloc(1, size);
    if (!image.data) {
        fprintf(stderr, "Failed to allocate memory for image data\n");
        return image;
    }
    
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t dest_idx = (y * width + x) * 4;
            size_t src_idx = position + (y * width + x) * 3;
            
            uint8_t r = buffer[src_idx];
            uint8_t g = buffer[src_idx + 1];
            uint8_t b = buffer[src_idx + 2];
            
            image.data[dest_idx] = b;
            image.data[dest_idx + 1] = g;
            image.data[dest_idx + 2] = r;
            image.data[dest_idx + 3] = 255;
        }
    }
    
    image.valid = true;
    return image;
}
