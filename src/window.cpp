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

