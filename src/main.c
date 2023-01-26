// Copyright (c) 2023 licktheroom //

// DEFINES //

#define WN_NAME "xcb-window"

// HEADERS //

// STANDARD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
    // Set basic window data
    game.window.width = game.window.height = 300;
    game.should_close = false;

    // Init
    if(!init()) {
        fprintf(stderr, "Init failed!\n");
        clean_up();

        return -1;
    }

    fprintf(stdout, "init done\n");
    
    // Wait until we should close
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

// As far as I could find, there are very few recources on XCB error codes
// All I can say is look through xproto.h or pray google finds it
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
    // Create window
    if(!window_create()) {
        fprintf(stderr, "Window creation failed!\n");
        return false;
    }

    // Get close event
    if(!window_get_close_event()) {
        fprintf(stderr, "Failed to get XCB window close event!\n");
        return false;
    }

    return true;
}

// See https://xcb.freedesktop.org/tutorial/events/
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
            // if it's a message about our window 
            case XCB_CLIENT_MESSAGE:
            {
                unsigned int ev = 
                        (*(xcb_client_message_event_t *)event).data.data32[0];

                // if it's the close event
                if(ev == game.xcb.close_event)
                    game.should_close = true;
            }

            break;
        }

        free(event); // Always free your event!
    }
}

// See https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
// and https://xcb.freedesktop.org/tutorial/events/
// and https://xcb.freedesktop.org/windowcontextandmanipulation/
bool
window_create(void)
{
    // Create the connection
    game.xcb.connection = xcb_connect(NULL, NULL);

    if(!game.xcb.connection) {
        fprintf(stderr, "Failed to create connection to Xorg!\n");

        return false;
    }

    // Error data, used later
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;

    // Get screen
    const xcb_setup_t *setup = xcb_get_setup(game.xcb.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    // Create window
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
    // Check if there was an error
    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to create window!\n");

        error_print(error);

        free(error);
        return false;
    }
    
    // Map the window
    cookie = xcb_map_window_checked(game.xcb.connection, game.xcb.window);

    // Check for error
    error = xcb_request_check(game.xcb.connection, cookie);

    if(error != NULL)
    {
        fprintf(stderr, "Failed to map window!\n");

        error_print(error);

        free(error);
        return false;
    }

    // Set the window's name
    cookie = xcb_change_property_checked(game.xcb.connection,
                                         XCB_PROP_MODE_REPLACE,
                                         game.xcb.window,
                                         XCB_ATOM_WM_NAME,
                                         XCB_ATOM_STRING,
                                         8,
                                         strlen(WN_NAME),
                                         WN_NAME);

    // Check for error
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

// See https://marc.info/?l=freedesktop-xcb&m=129381953404497
// Sadly that is the best result I could find
bool
window_get_close_event(void)
{
    xcb_generic_error_t *error;

    // We need to get WM_PROTOCOLS before we can get the close event
    xcb_intern_atom_cookie_t c_proto = xcb_intern_atom(game.xcb.connection,
                                                       1, 12,
                                                       "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *r_proto = 
                                    xcb_intern_atom_reply(game.xcb.connection,
                                                          c_proto,
                                                          &error);

    // Check for error
    if(error != NULL)
    {
        fprintf(stderr, "Failed to get WM_PROTOCOLS!\n");

        error_print(error);

        free(error);
        return false;
    }

    // Get the close event
    xcb_intern_atom_cookie_t c_close = xcb_intern_atom(game.xcb.connection,
                                                       0, 16,
                                                       "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t *r_close = 
                                    xcb_intern_atom_reply(game.xcb.connection,
                                                          c_close,
                                                          &error);

    // Check for error
    if(error != NULL)
    {
        fprintf(stderr, "Failed to get WM_DELETE_WINDOW!\n");

        error_print(error);

        free(error);
        return false;
    }

    // Enable the close event so we can actually receive it
    xcb_void_cookie_t cookie = xcb_change_property_checked(game.xcb.connection,
                                                           XCB_PROP_MODE_REPLACE,
                                                           game.xcb.window,
                                                           r_proto->atom,
                                                           XCB_ATOM_ATOM,
                                                           32,
                                                           1,
                                                           &r_close->atom);

    // Check for error
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
