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

    dither = sixel_dither_get(SIXEL_BUILTIN_XTERM256);
    sixel_output_new(&output, sixel_draw_callback, &stream, nullptr);
    sixel_output_set_palette_type(output, SIXEL_PALETTETYPE_AUTO);
    sixel_output_set_encode_policy(output, SIXEL_ENCODEPOLICY_FAST);
}

SixelCanvas::~SixelCanvas()
{
    draw_thread.reset();
    if (stream.is_open()) stream.close();
    fs::remove(out_file);

    sixel_dither_destroy(dither);
    sixel_output_destroy(output);
}

auto SixelCanvas::create(int x, int y, int max_width, int max_height) -> void
{
    this->x = x + 1;
    this->y = y + 1;
    this->max_width = max_width;
    this->max_height = max_height;
}

auto SixelCanvas::draw(Image& image) -> void
{
    stream.open(out_file, std::ios::in | std::ios::out | std::ios::binary);

    if (image.framerate() == -1) {
        draw_frame(image);
        return;
    }

    // start drawing loop
    draw_thread = std::make_unique<std::jthread>([&] (std::stop_token token) {
        while (!token.stop_requested()) {
            draw_frame(image);
            image.next_frame();
            unsigned long duration = (1.0 / image.framerate()) * 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        }
    });
}

auto SixelCanvas::clear() -> void
{
    std::scoped_lock lock {draw_mutex};
    draw_thread.reset();
    stream.close();

    // clear terminal
    for (int i = y; i <= max_height; ++i) {
        move_cursor(i, x);
        std::cout << std::string(max_width, ' ');
    }
    std::cout << std::flush;
    move_cursor(y, x);
}

auto SixelCanvas::draw_frame(const Image& image) -> void
{
    std::scoped_lock lock {draw_mutex};
    if (!stream.is_open()) return;

    // output sixel content to file
    fs::resize_file(out_file, 0);
    stream.seekp(0);
    sixel_encode(const_cast<unsigned char*>(image.data()),
            image.width(),
            image.height(),
            3 /*unused*/, dither, output);

    // write whole file to stdout
    stream.clear();
    stream.seekg(0);
    move_cursor(y, x);
    std::cout << stream.rdbuf() << std::flush;
}

auto SixelCanvas::move_cursor(int row, int col) -> void
{
    std::cout << "\033[" << row << ";" << col << "f" << std::flush;
}
