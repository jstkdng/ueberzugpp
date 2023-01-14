#include "window.hpp"
#include "free_delete.hpp"

#include <xcb/xcb.h>

Window::Window(xcb_connection_t *connection, xcb_screen_t *screen, xcb_window_t parent):
connection(connection),
screen(screen),
parent(parent)
{
    this->create();
}

Window::~Window()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
}

auto Window::create() -> void
{
    unsigned int value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    unsigned int value_list[4] = {
        this->screen->black_pixel,
        this->screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE,
        this->screen->default_colormap
    };

    xcb_window_t wid = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
            this->screen->root_depth,
            wid,
            this->parent,
            800, 50,
            500, 500,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);

    this->window = wid;
}

auto Window::get_id() -> xcb_window_t
{
    return this->window;
}

