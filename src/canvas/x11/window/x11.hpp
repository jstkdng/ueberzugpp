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

#ifndef X11_WINDOW_H
#define X11_WINDOW_H

#include "image.hpp"
#include "util/ptr.hpp"
#include "window.hpp"

#include <xcb/xproto.h>
#include <xcb/xcb_image.h>
#include <spdlog/fwd.h>

class Dimensions;

class X11Window : public Window
{
public:
    X11Window(xcb_connection_t* connection, xcb_screen_t *screen,
            xcb_window_t window, xcb_window_t parent, std::shared_ptr<Image> image);
    ~X11Window() override;

    void draw() override;
    void generate_frame() override;
    void show() override;
    void hide() override;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    xcb_window_t window;
    xcb_window_t parent;
    xcb_gcontext_t gc;

    c_unique_ptr<xcb_image_t, xcb_image_destroy> xcb_image;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<Image> image;

    bool visible = false;

    void send_expose_event();
    void create();
    void change_title();
};

#endif
