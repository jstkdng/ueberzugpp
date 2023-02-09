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
#include "logging.hpp"

#include <memory>
#include <stdexcept>
#include <xcb/xcb_image.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

Image::Image(xcb_connection_t *connection,
        xcb_screen_t *screen,
        std::string const& filename,
        int width, int height):
connection(connection),
screen(screen),
max_width(width),
max_height(height)
{
    logger << "Loading file " << filename << std::endl;
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
    this->image = cv::imread(filename, cv::IMREAD_COLOR);
    if (this->image.empty()) throw std::runtime_error("Unsupported image!");
    this->width = this->image.cols;
    this->height = this->image.rows;
    // check width/height before anything
    unsigned long max_dim = (this->width >= this->height) ?
                            this->width : this->height;
    unsigned long new_width = 0, new_height = 0;
    double scale = static_cast<double>(this->max_width) / max_dim;
    if (!(this->width <= this->max_width && this->height <= this->max_height)) {
        if (this->width >= this->height) {
            new_width = this->max_width;
            new_height = this->height * scale;
        } else {
            new_height = this->max_width;
            new_width = this->width * scale;
        }
    }

    if (new_width != 0 || new_height != 0) {
        cv::resize(this->image, this->image,
                cv::Size(new_width, new_height),
                0, 0, cv::INTER_AREA);
        this->width = new_width;
        this->height = new_height;
    }
    // alpha channel required
    if (this->image.channels() <= 3) {
        cv::cvtColor(this->image, this->image, cv::COLOR_BGR2BGRA);
    }
    this->size = this->image.total() * this->image.elemSize();
}

void Image::create_xcb_image()
{
    auto ptr = malloc(this->size);
    this->xcb_image = xcb_image_create_native(this->connection,
            this->width,
            this->height,
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            ptr,
            this->size,
            this->image.ptr());
}

void Image::draw(xcb_window_t const& window)
{
    this->create_xcb_gc(window);
    xcb_image_put(this->connection, window, this->gc, this->xcb_image, 0, 0, 0);
}

