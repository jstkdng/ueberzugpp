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

#ifndef __KITTY_STDOUT__
#define __KITTY_STDOUT__

#include "window.hpp"

#include <memory>
#include <mutex>
#include <vector>

class Image;
class KittyChunk;

class Kitty : public Window
{
public:
    explicit Kitty(std::unique_ptr<Image> new_image, std::mutex& stdout_mutex);
    ~Kitty() override;

    void draw() override;
    void generate_frame() override;

private:
    std::string str;
    std::unique_ptr<Image> image;
    std::mutex& stdout_mutex;
    uint32_t id;
    int x;
    int y;

    auto process_chunks() -> std::vector<KittyChunk>;
};

#endif
