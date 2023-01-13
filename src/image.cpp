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

#include "image.hpp"

#include <cstdlib>
#include <memory>
#include <vips/vips8>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

using namespace vips;

Image::Image(xcb_connection_t *connection, xcb_screen_t *screen):
connection(connection),
screen(screen)
{}

Image::~Image()
{
    this->destroy();
}

void Image::create_xcb_gc(xcb_window_t &window)
{
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, window, 0, nullptr);
    this->gc = cid;
}

void Image::destroy()
{
    if (!this->xcb_image.get()) return;
    this->image.reset(nullptr);
    this->imgdata.reset(nullptr);
    this->xcb_image.reset(nullptr);

    xcb_free_gc(this->connection, this->gc);
}

void Image::load(std::string &filename)
{
    VImage thumbnail = VImage::thumbnail(filename.c_str(), 500);
    VImage srgb = thumbnail.colourspace(VIPS_INTERPRETATION_sRGB);
    std::vector<VImage> bands = srgb.bandsplit();
    // convert from RGB to BGR
    VImage tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;
    // ensure fourth channel
    if (!srgb.has_alpha()) bands.push_back(bands[2]);
    this->image = std::make_unique<VImage>(VImage::bandjoin(bands));
    this->create_xcb_image(filename);
}

void Image::create_xcb_image(std::string &filename)
{
    auto size = VIPS_IMAGE_SIZEOF_IMAGE(this->image->get_image());
    this->imgdata.reset(this->image->write_to_memory(&size));
    this->xcb_image.reset(xcb_image_create_native(this->connection,
            this->image->width(),
            this->image->height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            this->imgdata.get(),
            size,
            nullptr));
}

void Image::draw(xcb_window_t &window)
{
    if (!this->xcb_image.get()) return;
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image.get(), 0, 0, 0);
}

