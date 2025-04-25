#include "safe_cast.h"
#include "window/window.h"

#include <stdio.h>
#include <stdlib.h>

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
            size_t width  = safe_to_size_t(geometry.width);
            size_t bytes  = height * width * 4;
            uint8_t *data = calloc(1, bytes);

            mcc_window_put_image(window, data, geometry.width, geometry.height);

            free(data);
            break;
        }
    }

    mcc_window_free(window);

    printf("Bye!\n");

    return 0;
}
