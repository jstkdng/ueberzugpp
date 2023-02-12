// Display ImageLs inside a terminal
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

#include "imagel.hpp"

#include <memory>
#include <xcb/xcb_image.h>

ImageL::ImageL(xcb_connection_t *connection,
        xcb_screen_t *screen,
        std::string const& filename,
        int width, int height):
connection(connection),
screen(screen)
{
    //this->load(filename);
    this->image = Image::load(filename, width, height);
    this->create_xcb_image();
}

ImageL::~ImageL()
{
    xcb_image_destroy(this->xcb_image);
    xcb_free_gc(this->connection, this->gc);
}

void ImageL::create_xcb_gc(xcb_window_t const& window)
{
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, window, 0, nullptr);
    this->gc = cid;
}

void ImageL::create_xcb_image()
{
    auto ptr = malloc(image->size());
    this->xcb_image = xcb_image_create_native(this->connection,
            image->width(),
            image->height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            ptr,
            image->size(),
            image->data());
}

void ImageL::draw(xcb_window_t const& window)
{
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image, 0, 0, 0);
}

