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
#include "dimensions.hpp"

#include <sixel.h>
#include <memory>
#include <thread>
#include <mutex>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class SixelCanvas : public Canvas
{
public:
    SixelCanvas();
    ~SixelCanvas();

    auto init(const Dimensions& dimensions,
            std::shared_ptr<Image> image) -> void override;
    auto draw() -> void override;
    auto clear() -> void override;

private:
    sixel_dither_t *dither = nullptr;
    sixel_output_t *output;
    std::shared_ptr<Image> image;

    std::unique_ptr<std::jthread> draw_thread;
    std::mutex draw_mutex;
    std::fstream stream;
    fs::path out_file;

    int x;
    int y;
    int max_width = 0;
    int max_height = 0;

    auto draw_frame() -> void;
};


#endif
