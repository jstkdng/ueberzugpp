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

#ifndef __IMAGE__
#define __IMAGE__

#include <memory>
#include <string>
#include "terminal.hpp"

class Image
{
public:
    static auto load(const std::string& filename,
            int max_width, int max_height, const Terminal& terminal)
        -> std::shared_ptr<Image>;
    virtual ~Image() = default;

    virtual auto width() const -> int = 0;
    virtual auto height() const -> int = 0;
    virtual auto size() const -> unsigned long = 0;
    virtual auto data() const -> const unsigned char* = 0;

    virtual auto framerate() const -> int { return -1; }
    virtual auto next_frame() -> void {}
protected:
    auto get_new_sizes(int max_width, int max_height) -> std::pair<int, int>;
};

#endif
