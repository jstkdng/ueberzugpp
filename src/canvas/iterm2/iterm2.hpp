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

#ifndef ITERM2_CANVAS_H
#define ITERM2_CANVAS_H

#include "chunk.hpp"
#include "image.hpp"
#include "window.hpp"

#include <mutex>
#include <string>
#include <vector>

class Iterm2 : public Window
{
  public:
    Iterm2(std::unique_ptr<Image> new_image, std::mutex *stdout_mutex);
    ~Iterm2() override;

    void draw() override;
    void generate_frame() override{};

  private:
    std::unique_ptr<Image> image;
    std::mutex *stdout_mutex;
    std::string str;

    int x;
    int y;
    int horizontal_cells = 0;
    int vertical_cells = 0;

    static auto process_chunks(const std::string &filename, int chunk_size, size_t num_bytes)
        -> std::vector<std::unique_ptr<Iterm2Chunk>>;
};

#endif
