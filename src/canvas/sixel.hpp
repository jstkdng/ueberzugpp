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

#include <memory>
#include <thread>
#include <mutex>
#include <sstream>
#include <atomic>

#include <sixel.h>

class SixelCanvas : public Canvas
{
public:
    explicit SixelCanvas(std::mutex& img_lock);
    ~SixelCanvas() override;

    void init(const Dimensions& dimensions,
            std::shared_ptr<Image> image) override;
    void draw() override;
    void clear() override;

private:
    sixel_dither_t *dither;
    sixel_output_t *output;
    std::shared_ptr<Image> image;

    std::thread draw_thread;
    std::atomic<bool> can_draw {true};

    std::mutex& img_lock;
    std::stringstream ss;

    int x;
    int y;
    int max_width = 0;
    int max_height = 0;

    void draw_frame();
};


#endif
