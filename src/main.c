#include "linalg/constants.h"
#include "safe_cast.h"
#include "window/window.h"
#include "cpu_rasterizer/cpu_rasterizer.h"
#include "linalg/matrix.h"
#include "linalg/transformations.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Key definitions for XCB
#define KEY_LEFT 113
#define KEY_RIGHT 114

struct shader_data {
    mcc_vec3f *positions;
    mcc_vec4f *colors;
    size_t vertex_count;
    mcc_mat4f mvp;
};

/**
 * Number of varyings shared from the vertex to fragment shaders
 */
const uint32_t SHADERS_VARYING_COUNT = 1;

void my_vertex_shader_fn(struct mcc_cpurast_vertex_shader_input *input) {
    struct shader_data *data = input->o_in_data;

    mcc_vec3f pos3 = data->positions[input->in_vertex_idx];
    mcc_vec4f pos4 = (mcc_vec4f){{ pos3.x, pos3.y, pos3.z, 1.f }};
    input->out_position = mcc_mat4f_mul_vec4f(data->mvp, pos4);
    
    // input->out_position = (mcc_vec4f){{ pos3.x, pos3.y, pos3.z, 1.f }};
    input->r_out_varyings[0].vec4f = data->colors[input->in_vertex_idx];
}

void my_fragment_shader_fn(struct mcc_cpurast_fragment_shader_input *input) {
    input->out_color = input->r_in_varyings[0].vec4f;
}

int main() {
    struct mcc_window *window = mcc_window_create((struct mcc_create_window_cfg){
        .title = "MCC!",
        .height = 100,
        .width = 100,
        .min_height = 0,
        .min_width = 0,
    });

    mcc_window_open(window);

    mcc_vec3f vertices[] = {
        {{-1.f,-1.f, 0.f}},
        {{ 1.f,-1.f, 0.f}},
        {{-1.f, 1.f, 0.f}},
    };
    mcc_vec4f colors[] = {
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 0.f, 1.f, 0.f, 1.f }},
        {{ 0.f, 0.f, 1.f, 1.f }},
    };

    struct shader_data shader_data = {
        .positions = vertices,
        .colors = colors,
        .vertex_count = sizeof(vertices) / sizeof(*vertices),
        .mvp = mcc_mat4f_scale_xyz(0.5f, 0.5f, 0.5f),
    };

    struct mcc_vertex_shader vertex_shader = {
        .r_fn = my_vertex_shader_fn,
        .varying_count = SHADERS_VARYING_COUNT,
    };
    struct mcc_fragment_shader fragment_shader = {
        .r_fn = my_fragment_shader_fn,
        .varying_count = SHADERS_VARYING_COUNT,
    };

    struct mcc_cpurast_render_config render_config = {
        .r_attachment = NULL, // Set before each render

        .o_vertex_shader_data = &shader_data,
        .r_vertex_shader = &vertex_shader,

        .o_fragment_shader_data = &shader_data,
        .r_fragment_shader = &fragment_shader,

        .culling_mode = MCC_CPURAST_CULLING_MODE_NONE,
        .o_depth_comparison_fn = NULL,

        .vertex_count = shader_data.vertex_count,
    };
    
    float rotation_angle = 0.0f;
    const float rotation_delta = 0.1f; // Amount to rotate per key press

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
                rotation_angle -= rotation_delta;
                need_redraw = true;
            } else if (event.key_press.keycode == KEY_RIGHT) {
                rotation_angle += rotation_delta;
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
            auto geometry = mcc_window_get_geometry(window);

            size_t height = safe_to_size_t(geometry.height),
                   width  = safe_to_size_t(geometry.width),
                   bytes  = height * width * 4;

            /*
             * Update the model view projection matrix
             */
            mcc_mat4f model = mcc_mat4f_rotate_y(rotation_angle);
            // Position the camera at {0,0,-3} (so move all object to {0,0,3})
            mcc_mat4f view = mcc_mat4f_translate_z(-3.f);
            mcc_mat4f projection = mcc_mat4f_perspective(
                (float)width / (float)height,
                0.001f, 100.f,
                MCC_PIf / 2.f
            );

            shader_data.mvp = mcc_mat4f_mul(projection, mcc_mat4f_mul(view, model));

            /*
             * Update the config structs for the rendering and allocate image data
             */
            uint8_t *image_data = calloc(1, bytes);
            struct mcc_cpurast_rendering_attachment attachment = {
                .o_depth = NULL, // No depth buffer
                .o_color = &(struct mcc_cpurast_rendering_color_attachment) { .r_data = image_data, },
                .width = width,
                .height = height,
            };
            render_config.r_attachment = &attachment;

            mcc_cpurast_render(&render_config);

            mcc_window_put_image(window, image_data, geometry.width, geometry.height);

            free(image_data);
        }
    }

    mcc_window_free(window);

    printf("Bye!\n");

    return 0;
}
