#pragma once

#include <stdint.h>
#include <xcb/xcb.h>

struct mcc_window;

struct mcc_create_window_cfg {
    const char *title;
    uint16_t width;
    uint16_t height;
    uint16_t min_width;
    uint16_t min_height;
};

enum mcc_window_event_kind {
    MCC_WINDOW_EVENT_UNKNOWN,
    MCC_WINDOW_EVENT_DELETE_WINDOW,
    MCC_WINDOW_EVENT_KEY_PRESS,
    MCC_WINDOW_EVENT_KEY_RELEASE,
    MCC_WINDOW_EVENT_MOUSE_MOVE,
    MCC_WINDOW_EVENT_BUTTON_PRESS,
    MCC_WINDOW_EVENT_BUTTON_RELEASE,
    MCC_WINDOW_EVENT_EXPOSE,
};

struct mcc_window_event_delete_window {
    enum mcc_window_event_kind kind;
};

struct mcc_window_event_key_press {
    enum mcc_window_event_kind kind;
    uint8_t keycode;
};

struct mcc_window_event_key_release {
    enum mcc_window_event_kind kind;
    uint8_t keycode;
};

struct mcc_window_event_mouse_move {
    enum mcc_window_event_kind kind;
};

struct mcc_window_event_button_press {
    enum mcc_window_event_kind kind;
};

struct mcc_window_event_button_release {
    enum mcc_window_event_kind kind;
};

struct mcc_window_event_expose {
    enum mcc_window_event_kind kind;
};

union mcc_window_event {
    enum   mcc_window_event_kind           kind;
    struct mcc_window_event_delete_window  delete_window;
    struct mcc_window_event_key_press      key_press;
    struct mcc_window_event_key_release    key_release;
    struct mcc_window_event_mouse_move     mouse_move;
    struct mcc_window_event_button_press   button_press;
    struct mcc_window_event_button_release button_release;
    struct mcc_window_event_expose expose;
};

struct mcc_window_geometry {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
};

struct mcc_window *mcc_window_create(struct mcc_create_window_cfg);

struct mcc_window_geometry mcc_window_get_geometry(struct mcc_window *);

void mcc_window_open(struct mcc_window *);
void mcc_window_close(struct mcc_window *);

union mcc_window_event mcc_window_wait_next_event(struct mcc_window *w);

/**
 * Puts an image on the window (format should be BGRA and the alpha component is ignored)
 */
void mcc_window_put_image(struct mcc_window *, uint8_t *data, uint16_t width, uint16_t height);

/**
 * Requests a redraw from the window manager
 * This will generate an EXPOSE event for the window
 */
void mcc_window_request_redraw(struct mcc_window *);

void mcc_window_free(struct mcc_window *);
