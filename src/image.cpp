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

#include <filesystem>
#include <memory>
#include <iostream>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

namespace fs = std::filesystem;
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
    if (!this->xcb_image) return;
    this->xcb_image = nullptr;
    xcb_free_gc(this->connection, this->gc);
    free(this->imgdata);
    this->imgdata = nullptr;
}

void Image::load(std::string &filename)
{
    // TODO: CALCULATE THUMBNAIL WIDTH
    VImage thumbnail = VImage::thumbnail(filename.c_str(), 500);
    VImage srgb = thumbnail.colourspace(VIPS_INTERPRETATION_sRGB);
    std::vector<VImage> bands = srgb.bandsplit();
    // convert from RGB to BGR
    VImage tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;
    // ensure fourth channel
    if (!srgb.has_alpha()) bands.push_back(bands[2]);
    this->image = VImage::bandjoin(bands);
    this->create_xcb_image(filename);
}

void Image::create_xcb_image(std::string &filename)
{
    std::size_t size = fs::file_size(filename);
    // memory xcb reads from
    this->imgdata = this->image.write_to_memory(&size);
    //this->imgdata.reset(tmp);
    // memory xcb writes to
    this->imgmemory = std::make_unique<char[]>(size);
    this->xcb_image = xcb_image_create_native(this->connection,
            this->image.width(),
            this->image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            this->imgmemory.get(),
            size,
            static_cast<unsigned char*>(this->imgdata));
}

void Image::draw(xcb_window_t &window)
{
    if (!this->xcb_image) return;
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image, 0, 0, 0);
    //xcb_flush(this->connection);
}

