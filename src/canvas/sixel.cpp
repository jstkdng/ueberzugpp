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

#include <unordered_set>
#include <string>
#include <chrono>

auto SixelCanvas::is_supported(const Terminal& terminal) -> bool
{
    std::unordered_set<std::string> supported_terms {
        "contour", "foot", "xterm-256color"
    };
    return supported_terms.contains(terminal.name);
}

SixelCanvas::SixelCanvas()
{
    sixel_encoder_new(&encoder, nullptr);
    sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_8BIT_MODE, nullptr);
}

SixelCanvas::~SixelCanvas()
{
    sixel_encoder_unref(encoder);
}

auto SixelCanvas::create(int x, int y, int max_width, int max_height) -> void
{}

auto SixelCanvas::draw(Image& image) -> void
{
    draw_frame(image);
    // TODO: how to do this with sixel?
    /*if (image.framerate() == -1) {
        draw_frame(image);
        return;
    }
    draw_thread = std::make_unique<std::jthread>([&] (std::stop_token token) {
        while (!token.stop_requested()) {
            draw_frame(image);
            image.next_frame();
            unsigned long duration = (1.0 / image.framerate()) * 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        }
    });*/
}

auto SixelCanvas::clear() -> void
{
    draw_thread.reset();
}

auto SixelCanvas::draw_frame(const Image& image) -> void
{
    sixel_encoder_encode_bytes(encoder,
            const_cast<unsigned char*>(image.data()),
            image.width(),
            image.height(),
            SIXEL_PIXELFORMAT_BGRA8888,
            nullptr,
            (-1));
}
