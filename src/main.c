#include "window/window.h"

int main() {
    struct mcc_window *window = mcc_window_create((struct mcc_create_window_cfg){
        .title = "MCC!",
        .height = 100,
        .width = 100,
        .min_height = 0,
        .min_width = 0,
    });

    mcc_window_open(window);

    for (uint64_t i = 0; i < 1000000000; i++);

    mcc_window_free(window);

    return 0;
}
