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

#include <memory>
#include <vips/vips8>
#include <xcb/xcb_image.h>

using namespace vips;

Image::Image(xcb_connection_t *connection,
        xcb_screen_t *screen,
        std::string const& filename,
        int width, int height):
connection(connection),
screen(screen),
max_width(width),
max_height(height)
{
    this->load(filename);
    this->create_xcb_image();
}

Image::~Image()
{
    xcb_image_destroy(this->xcb_image);
    xcb_free_gc(this->connection, this->gc);
}

void Image::create_xcb_gc(xcb_window_t const& window)
{
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, window, 0, nullptr);
    this->gc = cid;
}

// X11 requires images to be in the BGRx format
void Image::load(std::string const& filename)
{
    // check width/height before anything
    auto check = VImage::new_from_file(filename.c_str());
    VImage img;
    // at least 3 bands are required
    if (check.width() < this->max_width && check.height() < this->max_height) {
        // thumbnail not required
        img = check.colourspace(VIPS_INTERPRETATION_sRGB);
    } else {
        img = VImage::thumbnail(filename.c_str(), max_width - 1).colourspace(VIPS_INTERPRETATION_sRGB);
    }
    // alpha channel required
    if (!img.has_alpha()) img = img.bandjoin(255);
    // convert from RGB to BGR
    auto bands = img.bandsplit();
    auto tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;

    this->image = std::make_unique<VImage>(VImage::bandjoin(bands));
    this->size = VIPS_IMAGE_SIZEOF_IMAGE(this->image->get_image());
}

void Image::create_xcb_image()
{
    this->xcb_image = xcb_image_create_native(this->connection,
            this->image->width(),
            this->image->height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            this->image->write_to_memory(&(this->size)),
            this->size,
            nullptr);
}

void Image::draw(xcb_window_t const& window)
{
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image, 0, 0, 0);
}

