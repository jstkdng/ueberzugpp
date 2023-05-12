// Display images inside a terminal
// Copyright (C) 2023  JustKidding
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "window.hpp"

#include <xcb/xcb.h>

Window::Window(xcb_connection_t *connection, xcb_screen_t *screen,
            xcb_window_t window, xcb_window_t parent,
            const Dimensions& dimensions, Image& image):
connection(connection),
screen(screen),
window(window),
parent(parent),
image(image),
dimensions(dimensions),
gc(xcb_generate_id(connection))
{
    logger = spdlog::get("X11");
    unsigned int value_mask =  XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    auto value_list = xcb_create_window_value_list_t {
        .background_pixel = screen->black_pixel,
        .border_pixel = screen->black_pixel,
        .event_mask = XCB_EVENT_MASK_EXPOSURE,
        .colormap = screen->default_colormap
    };

    int16_t xcoord = dimensions.xpixels();
    int16_t ycoord = dimensions.ypixels();
    logger->debug("Parent window: {}", parent);
    xcb_create_window_aux(connection,
            screen->root_depth,
            window,
            this->parent,
            xcoord, ycoord,
            image.width(), image.height(),
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            value_mask,
            &value_list);
    logger->debug("Created child window {} at ({},{})", window, xcoord, ycoord);
    xcb_create_gc(connection, gc, window, 0, nullptr);
    show();
}

void Window::toggle()
{
    if (visible) {
        xcb_unmap_window(connection, window);
    } else {
        xcb_map_window(connection, window);
    }
    visible = !visible;
    xcb_flush(connection);
}

void Window::show()
{
    if (visible) return;
    visible = true;
    xcb_map_window(connection, window);
    xcb_flush(connection);
}

void Window::hide()
{
    if (!visible) return;
    visible = false;
    xcb_unmap_window(connection, window);
    xcb_flush(connection);
}

auto Window::draw() -> void
{
    if (!xcb_image.get()) return;
    xcb_image_put(connection, window, gc, xcb_image.get(), 0, 0, 0);
    xcb_flush(connection);
}

void Window::generate_frame()
{
    xcb_image_buffer = std::make_unique<unsigned char[]>(image.size());
    xcb_image = unique_C_ptr<xcb_image_t> {
        xcb_image_create_native(connection,
            image.width(),
            image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            screen->root_depth,
            xcb_image_buffer.get(),
            image.size(),
            const_cast<unsigned char*>(image.data()))
    };
    send_expose_event();
}

Window::~Window()
{
    terminate_event_handler();
    xcb_destroy_window(connection, window);
    xcb_free_gc(connection, gc);
    xcb_flush(connection);
}

auto Window::terminate_event_handler() -> void
{
    send_expose_event(69, 420);
}

auto Window::send_expose_event(int x, int y) -> void
{
    auto e = std::make_unique<xcb_expose_event_t>();
    e->response_type = XCB_EXPOSE;
    e->window = window;
    e->x = x;
    e->y = y;
    xcb_send_event(connection, false, window,
            XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(e.get()));
    xcb_flush(this->connection);
}

