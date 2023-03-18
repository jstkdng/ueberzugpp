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

#include "sixel.hpp"
#include "os.hpp"
#include "util.hpp"

#include <string>
#include <chrono>
#include <iostream>

#include <unistd.h>
#include <cstdlib>

int sixel_draw_callback(char *data, int size, void* priv)
{
    auto stream = static_cast<std::fstream*>(priv);
    if (stream->is_open()) stream->write(data, size);
    return size;
}

SixelCanvas::SixelCanvas()
{
    std::string tmppath = (fs::temp_directory_path() / "sixelXXXXXX").string();
    char *name = tmppath.data();
    int fd = mkstemp(name);
    close(fd); // fd not required
    out_file = tmppath;

    sixel_output_new(&output, sixel_draw_callback, &stream, nullptr);
    sixel_output_set_encode_policy(output, SIXEL_ENCODEPOLICY_FAST);
}

SixelCanvas::~SixelCanvas()
{
    draw_thread.reset();
    if (stream.is_open()) stream.close();
    fs::remove(out_file);

    sixel_output_destroy(output);
    if (dither) sixel_dither_destroy(dither);
}

auto SixelCanvas::init(const Dimensions& dimensions,
        std::shared_ptr<Image> image) -> void
{
    x = dimensions.x + 1;
    y = dimensions.y + 1;
    max_width = dimensions.max_w;
    max_height = dimensions.max_h;
    this->image = image;

    // create dither and palette from image
    sixel_dither_new(&dither, -1, nullptr);
    sixel_dither_initialize(dither,
            const_cast<unsigned char*>(image->data()),
            image->width(),
            image->height(),
            SIXEL_PIXELFORMAT_RGB888,
            SIXEL_LARGE_LUM,
            SIXEL_REP_CENTER_BOX,
            SIXEL_QUALITY_HIGH);
}

auto SixelCanvas::draw() -> void
{
    stream.open(out_file, std::ios::in | std::ios::out | std::ios::binary);

    if (!image->is_animated()) {
        draw_frame();
        return;
    }

    // start drawing loop
    draw_thread = std::make_unique<std::jthread>([&] (std::stop_token token) {
        while (!token.stop_requested()) {
            draw_frame();
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

auto SixelCanvas::clear() -> void
{
    if (max_width == 0 && max_height == 0) return;
    std::scoped_lock lock {draw_mutex};
    draw_thread.reset();
    if (stream.is_open()) stream.close();
    if (dither) {
        sixel_dither_destroy(dither);
        dither = nullptr;
    }

    // clear terminal
    for (int i = y; i <= max_height; ++i) {
        util::move_cursor(i, x);
        std::cout << std::string(max_width, ' ');
    }
    std::cout << std::flush;
    util::move_cursor(y, x);
}

auto SixelCanvas::draw_frame() -> void
{
    std::scoped_lock lock {draw_mutex};
    if (!stream.is_open()) return;

    // output sixel content to file
    fs::resize_file(out_file, 0);
    stream.seekp(0);
    sixel_encode(const_cast<unsigned char*>(image->data()),
            image->width(),
            image->height(),
            3 /*unused*/, dither, output);

    // write whole file to stdout
    stream.clear();
    stream.seekg(0);
    util::move_cursor(y, x);
    std::cout << stream.rdbuf() << std::flush;
}
