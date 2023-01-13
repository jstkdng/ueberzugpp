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

#ifndef __IMAGE__
#define __IMAGE__

#include <string>
#include <xcb/xcb_image.h>
#include <vips/vips8>
#include <memory>

#include "free_delete.hpp"

class Image
{
public:
    Image(xcb_connection_t *connection, xcb_screen_t *screen);
    ~Image();
    void draw(xcb_window_t &window);
    void load(std::string &filename);
    void destroy();

private:
    void create_xcb_image(std::string &filename);
    void create_xcb_gc(xcb_window_t &window);

    xcb_gcontext_t gc;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    std::unique_ptr<xcb_image_t, free_delete> xcb_image;
    std::unique_ptr<void, free_delete> imgdata;
    std::unique_ptr<vips::VImage> image;
};

#endif
