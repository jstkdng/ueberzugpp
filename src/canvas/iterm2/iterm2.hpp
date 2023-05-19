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

#ifndef __ITERM2_CANVAS__
#define __ITERM2_CANVAS__

#include "canvas.hpp"

#include <string>
#include <vector>
#include <string_view>
#include <spdlog/spdlog.h>

class Iterm2Chunk;

class Iterm2Canvas : public Canvas
{
public:
    Iterm2Canvas();
    ~Iterm2Canvas() override = default;
    void init(const Dimensions& dimensions, std::unique_ptr<Image> new_image) override;
    void draw() override;
    void clear() override;
private:
    std::unique_ptr<Image> image;
    std::shared_ptr<spdlog::logger> logger;
    std::string str;

    int x;
    int y;
    int max_width = 0;
    int max_height = 0;

    void draw_frame();
    static auto process_chunks(std::string_view filename, int chunk_size, size_t num_bytes) -> std::vector<std::unique_ptr<Iterm2Chunk>>;
};

#endif
