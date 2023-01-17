#include "window.hpp"
#include "free_delete.hpp"

#include <xcb/xcb.h>

Window::Window(
        xcb_connection_t *connection,
        xcb_screen_t *screen,
        xcb_window_t parent,
        int x, int y, int max_width, int max_height
):
connection(connection),
screen(screen),
parent(parent),
width(max_width),
height(max_height)
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
            x, y,
            max_width, max_height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);

    this->window = wid;
    xcb_map_window(this->connection, this->window);
}

Window::~Window()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
}

auto Window::get_dimensions() -> std::pair<int, int>
{
    return std::make_pair(this->width, this->height);
}

auto Window::get_id() -> xcb_window_t
{
    return this->window;
}

