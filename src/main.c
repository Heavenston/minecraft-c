#include "safe_cast.h"
#include "window/window.h"
#include "cpu_rasterizer/cpu_rasterizer.h"

#include <stdio.h>
#include <stdlib.h>

struct shader_data {
    uint32_t unused;
};

/**
 * Number of varyings shared from the vertex to fragment shaders
 */
const uint32_t SHADERS_VARYING_COUNT = 1;

void my_vertex_shader_fn(struct mcc_cpurast_vertex_shader_input *input) {
    switch (input->in_vertex_idx) {
    case 0:
        input->out_position = (mcc_vec4f){{ -1., -1., 0., 1. }};
        input->r_out_varyings[0].vec4f = (mcc_vec4f){{ 0., 0., 0., 0. }};
        break;
    case 1:
        input->out_position = (mcc_vec4f){{ -1.,  1., 0., 1. }};
        input->r_out_varyings[0].vec4f = (mcc_vec4f){{ 0., 1., 0., 0. }};
        break;
    case 2:
        input->out_position = (mcc_vec4f){{  1., -1., 0., 1. }};
        input->r_out_varyings[0].vec4f = (mcc_vec4f){{ 1., 0., 0., 0. }};
        break;
    default:
        fprintf(stderr, "Tried to run vertex shader with out of bound vertex index\n");
        input->out_position = (mcc_vec4f){{}};
        break;
    }
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

    for (bool close = false; !close;) {
        union mcc_window_event event = mcc_window_wait_next_event(window);
        switch (event.kind) {
        case MCC_WINDOW_EVENT_UNKNOWN:
            break;
        case MCC_WINDOW_EVENT_DELETE_WINDOW:
            close = true;
            break;
        case MCC_WINDOW_EVENT_KEY_PRESS:
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
            auto geometry = mcc_window_get_geometry(window);
            size_t height = safe_to_size_t(geometry.height);
            size_t width = safe_to_size_t(geometry.width);

            struct shader_data shader_data = {};

            size_t bytes = height * width * 4;
            uint8_t *image_data = calloc(1, bytes);
            struct mcc_cpurast_rendering_color_attachment color_attachment = {
                .r_data = image_data,
            };

            struct mcc_cpurast_rendering_attachment attachment = {
                .o_depth = NULL,
                .o_color = &color_attachment,
                .width = safe_to_u32(width),
                .height = safe_to_u32(height),
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
                .r_attachment = &attachment,

                .o_vertex_shader_data = &shader_data,
                .r_vertex_shader = &vertex_shader,

                .o_fragment_shader_data = &shader_data,
                .r_fragment_shader = &fragment_shader,

                .culling_mode = MCC_CPURAST_CULLING_MODE_NONE,
                .o_depth_comparison_fn = NULL,

                .vertex_count = 3,
            };

            mcc_cpurast_render(&render_config);

            mcc_window_put_image(window, image_data, geometry.width, geometry.height);

            free(image_data);
            break;
        }
    }

    mcc_window_free(window);

    printf("Bye!\n");

    return 0;
}
