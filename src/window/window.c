#include "window.h"
#include "../safe_cast.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>
#include <xcb/xproto.h>

const uint32_t mouse_event_mask = XCB_EVENT_MASK_BUTTON_PRESS
    | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_1_MOTION
    | XCB_EVENT_MASK_BUTTON_2_MOTION | XCB_EVENT_MASK_BUTTON_3_MOTION
    | XCB_EVENT_MASK_BUTTON_4_MOTION | 0;
const uint32_t event_mask = XCB_EVENT_MASK_EXPOSURE
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |

    mouse_event_mask |

    0;

struct mcc_window {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t window_id;

    bool mapped;
};

xcb_atom_t intern_helper(xcb_connection_t* connection, const char* name)
{
    auto cookie = xcb_intern_atom(connection, 0, safe_to_u16(strlen(name)), name);
    auto reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

struct mcc_window *mcc_window_create(struct mcc_create_window_cfg cfg) {
    struct mcc_window *w = malloc(sizeof(*w));

    xcb_connection_t *connection = xcb_connect(NULL, NULL);

    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    xcb_window_t window_id;
    {
        window_id = xcb_generate_id(connection);

        uint32_t mask = XCB_CW_EVENT_MASK;
        uint32_t value_list[] = { event_mask };
        xcb_create_window(connection,                    /* connection */
                          XCB_COPY_FROM_PARENT,          /* depth */
                          window_id,                     /* window id           */
                          screen->root,                  /* parent window       */
                          0, 0,                          /* x, y                */
                          cfg.width, cfg.height,         /* width, height       */
                          10,                            /* border_width        */
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class */
                          screen->root_visual,           /* visual              */
                          mask, value_list);

        xcb_icccm_set_wm_name(connection, window_id,
                              XCB_ATOM_STRING,
                              // 8 because everyone uses 8, not sure why
                              8, safe_to_u32(strlen(cfg.title)), cfg.title);
        xcb_size_hints_t hints;
        hints.flags =
            XCB_ICCCM_SIZE_HINT_P_SIZE | XCB_ICCCM_SIZE_HINT_P_MIN_SIZE;
        hints.width = cfg.width;
        hints.height = cfg.height;
        hints.min_width = cfg.min_width;
        hints.min_height = cfg.min_height;

        xcb_icccm_set_wm_normal_hints_checked(connection, window_id, &hints);

        xcb_atom_t protocols[] = { intern_helper(connection, "WM_DELETE_WINDOW") };
        xcb_icccm_set_wm_protocols(
            connection, window_id, intern_helper(connection, "WM_PROTOCOLS"),
            sizeof(protocols) / sizeof(*protocols), protocols);
    }

    *w = (struct mcc_window){
        .connection = connection,
        .screen = screen,
        .window_id = window_id,

        .mapped = false,
    };

    return w;
}

void mcc_window_open(struct mcc_window *w) {
    assert(!w->mapped);
    xcb_map_window(w->connection, w->window_id);
    xcb_flush(w->connection);
    w->mapped = true;
}

void mcc_window_close(struct mcc_window *w) {
    assert(w->mapped);
    xcb_unmap_window(w->connection, w->window_id);
    xcb_flush(w->connection);
    w->mapped = false;
}

union mcc_window_event mcc_window_translate_event(struct mcc_window *w, xcb_generic_event_t *event) {
    switch (event->response_type & ~0x80)
    {
    case XCB_CLIENT_MESSAGE: {
        auto client_message = (xcb_client_message_event_t*)event;
        if (client_message->format == 32
            && client_message->type == intern_helper(w->connection, "WM_PROTOCOLS")
            && client_message->data.data32[0] == intern_helper(w->connection, "WM_DELETE_WINDOW")) {

            return (union mcc_window_event) {
                .delete_window = {
                    .kind = MCC_WINDOW_EVENT_DELETE_WINDOW,
                },
            };
        }
        break;
    }
    case XCB_KEY_PRESS: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_KEY_PRESS,
            },
        };
    }
    case XCB_KEY_RELEASE: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_KEY_RELEASE,
            },
        };
    }
    case XCB_MOTION_NOTIFY: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_MOUSE_MOVE,
            },
        };
    }
    case XCB_BUTTON_PRESS: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_BUTTON_PRESS,
            },
        };
    }
    case XCB_BUTTON_RELEASE: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_BUTTON_RELEASE,
            },
        };
    }
    case XCB_EXPOSE: {
        return (union mcc_window_event) {
            .expose = {
                .kind = MCC_WINDOW_EVENT_EXPOSE,
            },
        };
    }
    default:
        printf("Unknown XCB event: %u\n", event->response_type & ~0x80);
    }

    return (union mcc_window_event) {
        .kind = MCC_WINDOW_EVENT_UNKNOWN,
    };
}

union mcc_window_event mcc_window_wait_next_event(struct mcc_window *w) {
    for(;;) {
        auto xcb_event = xcb_wait_for_event(w->connection);
        union mcc_window_event mcc_event = mcc_window_translate_event(w, xcb_event);
        if (mcc_event.kind != MCC_WINDOW_EVENT_UNKNOWN)
            return mcc_event;
    }
}

void mcc_window_free(struct mcc_window *w) {
    xcb_disconnect(w->connection);
    free(w);
}
