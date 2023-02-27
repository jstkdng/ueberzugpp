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

#ifndef __WINDOW__
#define __WINDOW__

#include "image.hpp"
#include "dimensions.hpp"

#include <xcb/xproto.h>
#include <xcb/xcb_image.h>
#include <thread>

class Window
{
public:
    Window(xcb_window_t parent, const Dimensions& dimensions, Image& image);
    ~Window();

    void draw();

private:
    xcb_connection_t *connection;
    xcb_window_t parent;
    xcb_window_t window;
    xcb_screen_t *screen;
    xcb_gcontext_t gc;
    xcb_image_t *xcb_image = nullptr;

    Image& image;
    const Dimensions& dimensions;
    std::unique_ptr<std::thread> event_handler;

    void handle_events();
    void create_window();
    void create_gc();
    void terminate_event_handler();
    void send_expose_event(int x = 0, int y = 0);
};

#endif
