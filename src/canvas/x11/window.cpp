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
#include "dimensions.hpp"

#include <xcb/xcb.h>
#include <gsl/util>

Window::Window(xcb_connection_t *connection, xcb_screen_t* screen,
            xcb_window_t window, xcb_window_t parent,
            const Dimensions& dimensions, Image& image):
connection(connection),
screen(screen),
window(window),
parent(parent),
gc(xcb_generate_id(connection)),
image(image)
{
    logger = spdlog::get("X11");
    const uint32_t value_mask =  XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    struct xcb_create_window_value_list_t value_list;
    value_list.background_pixel = screen->black_pixel;
    value_list.border_pixel = screen->black_pixel;
    value_list.event_mask = XCB_EVENT_MASK_EXPOSURE;
    value_list.colormap = screen->default_colormap;

    const auto xcoord = gsl::narrow_cast<int16_t>(dimensions.xpixels() + dimensions.padding_horizontal);
    const auto ycoord = gsl::narrow_cast<int16_t>(dimensions.ypixels() + dimensions.padding_vertical);
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
    if (visible) {
        return;
    }
    visible = true;
    xcb_map_window(connection, window);
    xcb_flush(connection);
}

void Window::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    xcb_unmap_window(connection, window);
    xcb_flush(connection);
}

auto Window::draw() -> void
{
    if (xcb_image.get() == nullptr) {
        return;
    }
    xcb_image_put(connection, window, gc, xcb_image.get(), 0, 0, 0);
    xcb_flush(connection);
}

void Window::generate_frame()
{
    xcb_image_buffer = std::vector<unsigned char>(image.size(), 0);
    xcb_image = unique_C_ptr<xcb_image_t> {
        xcb_image_create_native(connection,
            image.width(),
            image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            screen->root_depth,
            xcb_image_buffer.data(),
            image.size(),
            const_cast<unsigned char*>(image.data()))
    };
    send_expose_event();
}

Window::~Window()
{
    xcb_destroy_window(connection, window);
    xcb_free_gc(connection, gc);
    xcb_flush(connection);
}

auto Window::terminate_event_handler() -> void
{
    send_expose_event(MAGIC_EXIT_NUM1, MAGIC_EXIT_NUM2);
}

auto Window::send_expose_event(int xcoord, int ycoord) -> void
{
    const auto event = std::make_unique<xcb_expose_event_t>();
    event->response_type = XCB_EXPOSE;
    event->window = window;
    event->x = xcoord;
    event->y = ycoord;
    xcb_send_event(connection, 0, window,
            XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(event.get()));
    xcb_flush(this->connection);
}

