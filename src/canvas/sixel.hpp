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

#ifndef __SIXEL_CANVAS__
#define __SIXEL_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "terminal.hpp"

#include <sixel.h>
#include <memory>
#include <thread>

class SixelCanvas : public Canvas
{
public:
    SixelCanvas();
    ~SixelCanvas();

    auto create(int x, int y, int max_width, int max_height) -> void override;
    auto draw(Image& image) -> void override;
    auto clear() -> void override;

    static auto is_supported(const Terminal& terminal) -> bool;

private:
    sixel_encoder_t *encoder;
    std::unique_ptr<std::jthread> draw_thread;

    auto draw_frame(const Image& image) -> void;
};


#endif
