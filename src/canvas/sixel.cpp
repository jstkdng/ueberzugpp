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
}

SixelCanvas::~SixelCanvas()
{
    sixel_encoder_unref(encoder);
}

auto SixelCanvas::create(int x, int y, int max_width, int max_height) -> void
{}

auto SixelCanvas::draw(const Image& image) -> void
{
    sixel_encoder_encode_bytes(encoder,
            const_cast<unsigned char*>(image.data()),
            image.width(),
            image.height(),
            SIXEL_PIXELFORMAT_BGRA8888,
            nullptr,
            (-1));
}

auto SixelCanvas::clear() -> void
{}
