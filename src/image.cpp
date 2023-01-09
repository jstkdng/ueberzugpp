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
#include <filesystem>
#include <memory>
#include <iostream>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>

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
    unsigned int value_mask = XCB_GC_GRAPHICS_EXPOSURES;
    unsigned int value_list[1] = {
        0
    };
    xcb_create_gc(this->connection, cid, window, value_mask, value_list);
    this->gc = cid;
}

void Image::destroy()
{
    if (this->xcb_image) {
        xcb_image_destroy(this->xcb_image);
        g_free(this->imgdata);
        xcb_free_gc(this->connection, this->gc);
        this->xcb_image = nullptr;
    }
}

void Image::load(std::string &filename)
{
    // TODO: CALCULATE THUMBNAIL WIDTH
    auto img = VImage::thumbnail(filename.c_str(), 500);
    this->sanitize_image(img);
    // convert RGB TO BGR
    this->create_xcb_image(filename, img);
}

void Image::sanitize_image(VImage &img)
{
    img = img.colourspace(VIPS_INTERPRETATION_sRGB);
    auto bands = img.bandsplit();
    // convert from RGB to BGR
    auto tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;
    // ensure BGRX
    if (!img.has_alpha()) {
        bands.push_back(bands[2]);
    }
    img = VImage::bandjoin(bands);
}

void Image::create_xcb_image(std::string &filename, VImage &img)
{
    std::size_t len = fs::file_size(filename);
    this->imgmemory = calloc(len, sizeof(char));
    this->imgdata = img.write_to_memory(&len);
    unsigned char *udata = static_cast<unsigned char*>(this->imgdata);
    this->xcb_image = xcb_image_create_native(this->connection,
            img.width(),
            img.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            this->imgmemory,
            len,
            udata);
}

void Image::draw(xcb_window_t &window)
{
    if (!this->xcb_image) return;
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image, 0, 0, 0);
}
