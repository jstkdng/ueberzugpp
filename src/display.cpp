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

#include <iostream>
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
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_free_colormap(this->connection, this->colormap);
    xcb_disconnect(this->connection);
}

void Display::set_screen()
{
    const xcb_setup_t *setup = xcb_get_setup(this->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    this->screen = iter.data;
}

void Display::create_colormap()
{
    xcb_colormap_t mid = xcb_generate_id(this->connection);
    xcb_create_colormap(this->connection,
            XCB_COLORMAP_ALLOC_NONE,
            mid,
            this->screen->root,
            this->screen->root_visual);
    this->colormap = mid;
}

void Display::create_window()
{
    this->create_colormap();

    unsigned int value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    unsigned int value_list[4] = {
        this->screen->black_pixel,
        this->screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS,
        this->colormap
    };

    xcb_window_t wid = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
            0,
            wid,
            this->screen->root,
            0, 0,
            400, 400,
            5,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);
    xcb_map_window(this->connection, wid);
    xcb_flush(this->connection);

    this->window = wid;
}

void Display::handle_events()
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete>
            event (xcb_wait_for_event(this->connection));
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                logger.log("Exposing window!");
                break;
            }
            case XCB_KEY_PRESS: {
                logger.log("Key pressed!");
                goto endloop;
            }
            default: {
                logger.log("Unknown event");
                break;
            }
        }
    }
    endloop:
    return;
}

