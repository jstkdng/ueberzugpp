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

class Image
{
public:
    Image(xcb_connection_t *connection,
            xcb_screen_t *screen,
            std::string const& filename,
            int width, int height);
    ~Image();
    void draw(xcb_window_t const& window);

private:
    void create_xcb_image();
    void create_xcb_gc(xcb_window_t const& window);
    void load(std::string const& filename);

    xcb_gcontext_t gc;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    xcb_image_t *xcb_image;
    std::unique_ptr<vips::VImage> image;
    int max_width;
    int max_height;
    unsigned long size;
};

#endif
