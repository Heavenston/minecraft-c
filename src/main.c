#include "linalg/constants.h"
#include "safe_cast.h"
#include "window/window.h"
#include "cpu_rasterizer/cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/transformations.h"
#include "chunk/chunk.h"
#include "chunk/triangulate.h"
#include "chunk/shader.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Key definitions for XCB
#define KEY_LEFT 113
#define KEY_RIGHT 114
#define KEY_UP 111
#define KEY_DOWN 116

static inline long long diff_ns(struct timespec start,
                                struct timespec end)
{
    return (long long)(end.tv_sec - start.tv_sec) * 1000000000LL
         + (end.tv_nsec - start.tv_nsec);
}

int main() {
    struct mcc_window *window = mcc_window_create((struct mcc_create_window_cfg){
        .title = "MCC Chunk Viewer",
        .height = 800,
        .width = 800,
        .min_height = 400,
        .min_width = 400,
    });

    mcc_window_open(window);

    // Generate a chunk with seed 0
    struct mcc_chunk_data chunk_data = {
        .x = 0,
        .y = 0,
        .z = 0
    };
    {
        struct timespec generate_start, generate_end;
        timespec_get(&generate_start, TIME_UTC);
        mcc_chunk_generate(0, &chunk_data);
        timespec_get(&generate_end, TIME_UTC);
        printf("Generated chunk in %fms\n", (double)diff_ns(generate_start, generate_end) / 1'000'000.);
    }
    
    // Create a mesh from the chunk data
    struct mcc_chunk_mesh chunk_mesh;
    mcc_chunk_mesh_init(&chunk_mesh);
    {
        struct timespec mesh_start, mesh_end;
        timespec_get(&mesh_start, TIME_UTC);
        mcc_chunk_mesh_create(&chunk_mesh, &chunk_data);
        timespec_get(&mesh_end, TIME_UTC);
        printf("Meshed chunk in %fms\n", (double)diff_ns(mesh_start, mesh_end) / 1'000'000.);
    }
    
    printf("Generated chunk mesh with %zu vertices\n", chunk_mesh.vertex_count);
    
    // Render object to pass to shader
    struct mcc_chunk_render_object render_object = {
        .data = mcc_chunk_render_data_load(),
        .mesh = &chunk_mesh,
        .mvp = mcc_mat4f_identity() // Initial MVP, will be updated each frame
    };

    struct mcc_cpurast_clear_config clear_config = {
        .clear_depth = 1.0f,
        .clear_color = { 0.4f, 0.6f, 0.9f, 1.0f }, // Sky blue
    };

    // Camera control variables
    float rotation_x = 0.0f;
    float rotation_y = 0.0f;
    
    const float rotation_delta = 0.1f;

    bool enable_wireframe = false,
         enable_depth_rendering = false;

    mcc_vec3f camera_pos = {{ 6.f, 5.5f, 6.f }};

    for (bool close = false; !close;) {
        union mcc_window_event event = mcc_window_wait_next_event(window);
        bool need_redraw = false;
        
        switch (event.kind) {
        case MCC_WINDOW_EVENT_UNKNOWN:
            break;
        case MCC_WINDOW_EVENT_DELETE_WINDOW:
            close = true;
            break;
        case MCC_WINDOW_EVENT_KEY_PRESS:
            printf("Key pressed: %u\n", event.key_press.keycode);
            if (event.key_press.keycode == KEY_LEFT) {
                rotation_y -= rotation_delta;
                need_redraw = true;
            } else if (event.key_press.keycode == KEY_RIGHT) {
                rotation_y += rotation_delta;
                need_redraw = true;
            } else if (event.key_press.keycode == KEY_UP) {
                rotation_x -= rotation_delta;
                need_redraw = true;
            } else if (event.key_press.keycode == KEY_DOWN) {
                rotation_x += rotation_delta;
                need_redraw = true;
            } else if (event.key_press.keycode == 25 /* 'w' */) {
                enable_wireframe = !enable_wireframe;
                need_redraw = true;
            } else if (event.key_press.keycode == 40 /* 'd' */) {
                enable_depth_rendering = !enable_depth_rendering;
                need_redraw = true;
            }
            break;
        case MCC_WINDOW_EVENT_KEY_RELEASE:
            break;
        case MCC_WINDOW_EVENT_MOUSE_MOVE:
            break;
        case MCC_WINDOW_EVENT_BUTTON_PRESS:
            break;
        case MCC_WINDOW_EVENT_BUTTON_RELEASE:
            break;
        case MCC_WINDOW_EVENT_EXPOSE:
            need_redraw = true;
            break;
        }
        
        if (need_redraw) {
            struct timespec render_start, render_end;
            timespec_get(&render_start, TIME_UTC);

            auto geometry = mcc_window_get_geometry(window);

            size_t height = safe_to_size_t(geometry.height),
                   width  = safe_to_size_t(geometry.width);

            /*
             * Update the model view projection matrix
             * Position the camera to see the bottom of the chunk
             */
            
            // Model transformations - center the chunk and scale it appropriately
            mcc_mat4f model = mcc_mat4f_identity();
            
            // View transformations - position camera to look at the bottom of the chunk
            mcc_mat4f view = mcc_mat4f_identity();
            view = mcc_mat4f_mul(view, mcc_mat4f_translate(mcc_vec3f_scale(camera_pos, +1.f)));
            view = mcc_mat4f_mul(view, mcc_mat4f_rotate_y(rotation_y));
            view = mcc_mat4f_mul(view, mcc_mat4f_rotate_x(rotation_x));
            view = mcc_mat4f_inverse(view);
            
            // Projection
            mcc_mat4f projection = mcc_mat4f_perspective(
                (float)width / (float)height,
                0.1f, 20.0f,
                MCC_PIf / 2.0f
            );

            render_object.mvp = mcc_mat4f_mul(projection, mcc_mat4f_mul(view, model));

            /*
             * Allocate image and depth buffers
             */
            uint8_t *image_data = malloc(width * height * sizeof(*image_data) * 4);
            float32_t *depth_data = malloc(width * height * sizeof(*depth_data));
            struct mcc_cpurast_rendering_attachment attachment = {
                .o_depth = &(struct mcc_cpurast_rendering_depth_attachment) { .r_data = depth_data },
                .o_color = &(struct mcc_cpurast_rendering_color_attachment) { .r_data = image_data, },
                .width = safe_to_u32(width),
                .height = safe_to_u32(height),
            };
            
            // Setup render configuration using our chunk shaders
            struct mcc_cpurast_render_config render_config;
            mcc_chunk_render_config(
                &render_config,
                &render_object,
                &attachment
            );

            if (enable_wireframe) {
                render_config.culling_mode = MCC_CPURAST_CULLING_MODE_NONE;
                render_config.polygon_mode = MCC_CPURAST_POLYGON_MODE_LINE;
            }
            
            clear_config.r_attachment = &attachment;

            // Render the chunk
            mcc_cpurast_clear(&clear_config);
            mcc_cpurast_render(&render_config);

            if (enable_depth_rendering) {
                float max_depth = -1.f, min_depth = 1.f;
                for (size_t i = 0; i < width*height; i++) {
                    if (depth_data[i] > max_depth)
                        max_depth = depth_data[i];
                    if (depth_data[i] < min_depth)
                        min_depth = depth_data[i];
                }
                for (size_t i = 0; i < width*height; i++) {
                    uint8_t p = 255 - (uint8_t)(((depth_data[i] - min_depth) / (max_depth - min_depth)) * 255.f);
                    memset(image_data + i*4, p, 4);
                }
            }

            mcc_window_put_image(window, image_data, geometry.width, geometry.height);

            // Clean up resources
            mcc_chunk_render_config_cleanup(&render_config);
            free(image_data);
            free(depth_data);

            timespec_get(&render_end, TIME_UTC);
            printf("Finished rendering (took %fms)!\n", (double)diff_ns(render_start, render_end) / 1'000'000.);
        }
    }

    // Clean up
    mcc_chunk_mesh_free(&chunk_mesh);
    mcc_window_free(window);

    mcc_chunk_render_data_free(render_object.data);

    printf("Bye!\n");

    return 0;
}
