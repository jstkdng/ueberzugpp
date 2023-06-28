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

#include "x11.hpp"
#include "dimensions.hpp"

#include <xcb/xcb.h>
#include <gsl/gsl>
#include <string_view>

constexpr std::string_view win_name = "ueberzugpp";

X11Window::X11Window(xcb_connection_t* connection, xcb_screen_t* screen,
            xcb_window_t window, xcb_window_t parent, const Image& image):
connection(connection),
screen(screen),
window(window),
parent(parent),
gc(xcb_generate_id(connection)),
image(image)
{
    logger = spdlog::get("X11");
    create();
    change_title();
    show();
}

void X11Window::create()
{
    const uint32_t value_mask =  XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    struct xcb_create_window_value_list_t value_list;
    value_list.background_pixel = screen->black_pixel;
    value_list.border_pixel = screen->black_pixel;
    value_list.event_mask = XCB_EVENT_MASK_EXPOSURE;
    value_list.colormap = screen->default_colormap;

    const auto dimensions = image.dimensions();
    const auto xcoord = gsl::narrow_cast<int16_t>(dimensions.xpixels() + dimensions.padding_horizontal);
    const auto ycoord = gsl::narrow_cast<int16_t>(dimensions.ypixels() + dimensions.padding_vertical);
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

    xcb_create_gc(connection, gc, window, 0, nullptr);
    logger->debug("Created child window {} at ({},{}) with parent {}", window, xcoord, ycoord, parent);
}

void X11Window::change_title()
{
    const int bits_in_char = 8;
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
            window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, bits_in_char, win_name.size(), win_name.data());
}

void X11Window::show()
{
    if (visible) {
        return;
    }
    visible = true;
    xcb_map_window(connection, window);
    xcb_flush(connection);
}

void X11Window::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    xcb_unmap_window(connection, window);
    xcb_flush(connection);
}

void X11Window::draw()
{
    if (!xcb_image) {
        return;
    }
    xcb_image_put(connection, window, gc, xcb_image.get(), 0, 0, 0);
}

void X11Window::generate_frame()
{
    xcb_image.reset(
        xcb_image_create_native(connection,
            image.width(),
            image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            screen->root_depth,
            nullptr, 0, nullptr)
    );
    xcb_image->data = const_cast<unsigned char*>(image.data());
    send_expose_event();
}

X11Window::~X11Window()
{
    xcb_destroy_window(connection, window);
    xcb_free_gc(connection, gc);
    xcb_flush(connection);
}

void X11Window::send_expose_event()
{
    const int event_size = 32;
    std::array<char, event_size> buffer;
    auto *event = reinterpret_cast<xcb_expose_event_t*>(buffer.data());
    event->response_type = XCB_EXPOSE;
    event->window = window;
    xcb_send_event(connection, 0, window,
            XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(event));
    xcb_flush(connection);
}

