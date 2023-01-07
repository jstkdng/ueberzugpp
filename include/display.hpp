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

#ifndef __DISPLAY__
#define __DISPLAY__

#include "logging.hpp"

#include <memory>
#include <string>
#include <vips/vips8>
#include <xcb/xproto.h>
#include <xcb/xcb_image.h>

class Display
{
public:
    Display(Logging &logger, std::string &filename);
    ~Display();

    void create_window();
    void handle_events();
    void draw_image();

private:
    void set_screen();
    void create_colormap();
    void create_gc();
    void create_pixmap();
    void create_xcb_image();

    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_image_t *xcb_image = nullptr;

    xcb_window_t window;
    xcb_colormap_t colormap;
    xcb_pixmap_t pixmap;
    xcb_gcontext_t gc;

    vips::VImage image;

    Logging &logger;
    std::string &filename;
};


#endif
