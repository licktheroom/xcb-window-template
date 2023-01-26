// DEFINES //

#define DEBUG
#define WN_NAME "xcb-window"

// HEADERS //

// STANDARD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

// X11

#include <xcb/xcb.h>

// STATIC VARIABLES //

static struct 
{
    struct
    {
        xcb_connection_t *connection;
        xcb_window_t window;

        xcb_atom_t close_event;
    } xcb;

    bool should_close;

    struct
    {
        int width, height;
    } window;
} game;

// FUNCTIONS //

void
clean_up(void);

void
error_print(xcb_generic_error_t *error);

bool 
init(void);

void
input(void);

bool
window_create(void);

bool
window_get_close_event(void);

// MAIN //

int
main()
{
    game.window.width = game.window.height = 300;
    game.should_close = false;

    if(!init()) {
        fprintf(stderr, "Init failed!\n");
        clean_up();

        return -1;
    }

    fprintf(stdout, "init done\n");
    
    while(game.should_close == false)
    {
        input();
    }
    
    clean_up();
    return 0;
}

// FUNCTIONS //

void
clean_up(void)
{
    xcb_destroy_window(game.xcb.connection, game.xcb.window);
    xcb_disconnect(game.xcb.connection);
}

void
error_print(xcb_generic_error_t *error)
{
    if(error == NULL)
        return;

    const char *names[] = {
        "XCB_REQUEST",
        "XCB_VALUE",
        "XCB_WINDOW",
        "XCB_PIXMAP",
        "XCB_ATOM",
        "XCB_CURSOR",
        "XCB_FONT",
        "XCB_MATCH",
        "XCB_DRAWABLE",
        "XCB_ACCESS",
        "XCB_ALLOC",
        "XCB_COLORMAP",
        "XCB_G_CONTEXT",
        "XCB_ID_CHOICE",
        "XCB_NAME",
        "XCB_LENGTH",
        "XCB_IMPLEMENTATION"
    };

    switch(error->error_code)
    {
        case XCB_REQUEST:
        case XCB_MATCH:
        case XCB_ACCESS:
        case XCB_ALLOC:
        case XCB_NAME:
        case XCB_LENGTH:
        case XCB_IMPLEMENTATION:
            {
                xcb_request_error_t *er = (xcb_request_error_t *)error;

                fprintf(stderr, "REQUEST ERROR\n"
                                "%s\n"
                                "error_code: %d\n"
                                "major: %d\n"
                                "minor: %d\n",
                                names[er->error_code],
                                er->error_code,
                                er->major_opcode,
                                er->minor_opcode);
            }
            break;

        case XCB_VALUE:
        case XCB_WINDOW:
        case XCB_PIXMAP:
        case XCB_ATOM:
        case XCB_CURSOR:
        case XCB_FONT:
        case XCB_DRAWABLE:
        case XCB_COLORMAP:
        case XCB_G_CONTEXT:
        case XCB_ID_CHOICE:
            {
                xcb_value_error_t *er = (xcb_value_error_t *)error;

                fprintf(stderr, "VALUE ERROR\n"
                                "%s\n"
                                "error_code: %d\n"
                                "major: %d\n"
                                "minor: %d\n",
                                names[er->error_code],
                                er->error_code,
                                er->major_opcode,
                                er->minor_opcode);
            }
            break;
    }
}

bool
init(void)
{
    if(!window_create()) {
        fprintf(stderr, "Window creation failed!\n");
        return false;
    }

    if(!window_get_close_event()) {
        fprintf(stderr, "Failed to get XCB window close event!\n");
        return false;
    }

    return true;
}

void
input(void)
{
    while(true)
    {
        xcb_generic_event_t *event = xcb_poll_for_event(game.xcb.connection);

        if(event == NULL)
            break;

        switch(event->response_type & ~0x80)
        {
            case XCB_CLIENT_MESSAGE:
            {
                unsigned int ev = 
                        (*(xcb_client_message_event_t *)event).data.data32[0];

                if(ev == game.xcb.close_event)
                    game.should_close = true;
            }

            break;
        }

        free(event);
    }
}

bool
window_create(void)
{
    game.xcb.connection = xcb_connect(NULL, NULL);

    if(!game.xcb.connection) {
        fprintf(stderr, "Failed to create connection to Xorg!\n");

        return false;
    }

    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;

    const xcb_setup_t *setup = xcb_get_setup(game.xcb.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    const int eventmask = XCB_EVENT_MASK_EXPOSURE | 
                          XCB_EVENT_MASK_KEY_PRESS | 
                          XCB_EVENT_MASK_KEY_RELEASE | 
                          XCB_EVENT_MASK_BUTTON_PRESS | 
                          XCB_EVENT_MASK_BUTTON_RELEASE | 
                          XCB_EVENT_MASK_POINTER_MOTION | 
                          XCB_EVENT_MASK_BUTTON_MOTION;

    const int valwin[] = {eventmask, 0};
    const int valmask = XCB_CW_EVENT_MASK;

    game.xcb.window = xcb_generate_id(game.xcb.connection);

    cookie = xcb_create_window_checked(game.xcb.connection,
                                       XCB_COPY_FROM_PARENT,
                                       game.xcb.window,
                                       screen->root,
                                       0, 0,
                                       game.window.width, game.window.height,
                                       10,
                                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                       screen->root_visual,
                                       valmask, valwin);

    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to create window!\n");

        error_print(error);

        free(error);
        return false;
    }

    cookie = xcb_map_window_checked(game.xcb.connection, game.xcb.window);

    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to map window!\n");

        error_print(error);

        free(error);
        return false;
    }

    cookie = xcb_change_property_checked(game.xcb.connection,
                                         XCB_PROP_MODE_REPLACE,
                                         game.xcb.window,
                                         XCB_ATOM_WM_NAME,
                                         XCB_ATOM_STRING,
                                         8,
                                         strlen(WN_NAME),
                                         WN_NAME);

    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to rename window!\n");

        error_print(error);

        free(error);
        return false;
    }

    return true;
}

bool
window_get_close_event(void)
{
    xcb_generic_error_t *error;

    xcb_intern_atom_cookie_t c_proto = xcb_intern_atom(game.xcb.connection,
                                                       1, 12,
                                                       "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *r_proto = 
                                    xcb_intern_atom_reply(game.xcb.connection,
                                                          c_proto,
                                                          &error);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to get WM_PROTOCOLS!\n");

        error_print(error);

        free(error);
        return false;
    }

    xcb_intern_atom_cookie_t c_close = xcb_intern_atom(game.xcb.connection,
                                                       0, 16,
                                                       "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t *r_close = 
                                    xcb_intern_atom_reply(game.xcb.connection,
                                                          c_close,
                                                          &error);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to get WM_DELETE_WINDOW!\n");

        error_print(error);

        free(error);
        return false;
    }

    xcb_void_cookie_t cookie = xcb_change_property_checked(game.xcb.connection,
                                                           XCB_PROP_MODE_REPLACE,
                                                           game.xcb.window,
                                                           r_proto->atom,
                                                           XCB_ATOM_ATOM,
                                                           32,
                                                           1,
                                                           &r_close->atom);

    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to set window close event!\n");

        error_print(error);

        free(error);
        return false;
    }

    game.xcb.close_event = r_close->atom;

    return true;
}