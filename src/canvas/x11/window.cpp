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
#include "logging.hpp"

#include <xcb/xcb.h>

struct free_delete
{
    void operator()(void* x) { free(x); }
};

Window::Window(xcb_connection_t *connection, xcb_window_t parent, xcb_screen_t *screen,
           const Dimensions& dimensions, std::shared_ptr<Image> image):
connection(connection),
parent(parent),
image(image),
screen(screen),
window(xcb_generate_id(connection)),
gc(xcb_generate_id(connection))
{
    unsigned int value_mask =  XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    auto value_list = std::make_unique<xcb_create_window_value_list_t>();
    value_list->background_pixel =  screen->black_pixel;
    value_list->border_pixel = screen->black_pixel;
    value_list->event_mask = XCB_EVENT_MASK_EXPOSURE;
    value_list->colormap = screen->default_colormap;

    xcb_create_window_aux(connection,
            screen->root_depth,
            window,
            this->parent,
            dimensions.xpixels(), dimensions.ypixels(),
            image->width(), image->height(),
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            value_mask,
            value_list.get());
    xcb_map_window(connection, window);
    xcb_create_gc(connection, gc, window, 0, nullptr);

    event_handler = std::make_unique<std::thread>([this] {
        handle_events();
    });

    xcb_flush(connection);
}

auto Window::draw() -> void
{
    if (!image->is_animated()) {
        draw_frame();
        return;
    }
    draw_thread = std::make_unique<std::jthread>([&] (std::stop_token token) {
        while (!token.stop_requested()) {
            draw_frame();
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

auto Window::draw_frame() -> void
{
    if (xcb_image) xcb_image_destroy(xcb_image);
    auto ptr = malloc(image->size());
    xcb_image = xcb_image_create_native(connection,
            image->width(),
            image->height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            screen->root_depth,
            ptr,
            image->size(),
            const_cast<unsigned char*>(image->data()));
    send_expose_event();
}

Window::~Window()
{
    terminate_event_handler();
    if (event_handler->joinable()) event_handler->join();
    if (xcb_image) xcb_image_destroy(xcb_image);
    xcb_free_gc(connection, gc);
    xcb_unmap_window(connection, window);
    xcb_destroy_window(connection, window);
    xcb_flush(connection);
}

auto Window::handle_events() -> void
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete> event {
            xcb_wait_for_event(this->connection)
        };
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                auto expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                if (expose->x == 69 && expose->y == 420) return;
                if (!xcb_image) continue;
                xcb_image_put(connection, window, gc, xcb_image, 0, 0, 0);
                break;
            }
            default: {
                break;
            }
        }
    }
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

