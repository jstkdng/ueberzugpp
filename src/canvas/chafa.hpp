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

#ifndef __CHAFA_CANVAS__
#define __CHAFA_CANVAS__

#include "canvas.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <chafa.h>

class ChafaCanvas : public Canvas
{
public:
    ChafaCanvas();
    ~ChafaCanvas() override;

    void init(const Dimensions& dimensions, std::unique_ptr<Image> new_image) override;
    void draw() override;
    void clear() override;

private:
    ChafaSymbolMap* symbol_map = nullptr;
    ChafaCanvasConfig* config = nullptr;
    ChafaCanvas* canvas = nullptr;
    ChafaTermInfo* term_info = nullptr;

    std::unique_ptr<Image> image;
    std::shared_ptr<spdlog::logger> logger;

    int x;
    int y;
    int horizontal_cells = 0;
    int vertical_cells = 0;
};

#endif
