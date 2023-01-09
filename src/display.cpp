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

#include <cstdlib>
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "display.hpp"

struct free_delete
{
    void operator()(void *x) { free(x); }
};

Display::Display(Logging &logger):
logger(logger)
{
    this->connection = xcb_connect(NULL, NULL);
    this->set_screen();
    image = std::make_unique<Image>(this->connection, this->screen);
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_disconnect(this->connection);
}

void Display::set_screen()
{
    const xcb_setup_t *setup = xcb_get_setup(this->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    this->screen = iter.data;
}

void Display::destroy_image()
{
    image->destroy();
    xcb_clear_area(this->connection, false, this->window, 0, 0, 0, 0);
    xcb_flush(this->connection);
}

void Display::load_image(std::string filename)
{
    image->load(filename);
    this->trigger_redraw();
}

void Display::trigger_redraw()
{
    xcb_expose_event_t *e = static_cast<xcb_expose_event_t*>(calloc(1, sizeof(xcb_expose_event_t)));
    e->response_type = XCB_EXPOSE;
    e->window = this->window;
    e->x = 0;
    e->y = 0;
    e->width = 0;
    e->height = 0;
    e->count = 0;
    xcb_send_event(this->connection, false, this->window, XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(e));
    xcb_flush(this->connection);
}

void Display::create_window()
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
            this->screen->root,
            0, 0,
            1000, 1000,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);
    xcb_map_window(this->connection, wid);
    xcb_flush(this->connection);

    this->window = wid;
}

std::thread Display::spawn_event_handler()
{
    return std::thread([this] {
        this->handle_events();
    });
}

void Display::handle_events()
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete>
            event (xcb_wait_for_event(this->connection));
        auto response = event->response_type & ~0x80;
        switch (response) {
            case XCB_EXPOSE: {
                if (this->image) this->image->draw(this->window);
                break;
            }
            default: {
                break;
            }
        }
    }
}

