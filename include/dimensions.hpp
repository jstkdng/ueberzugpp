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

#ifndef __DIMENSIONS__
#define __DIMENSIONS__

#include "terminal.hpp"

class Dimensions
{
public:
    Dimensions(const Terminal& terminal, int x, int y, int max_w, int max_h, const std::string& scaler);
    ~Dimensions() = default;

    void reload();

    int xpixels() const;
    int ypixels() const;
    int max_wpixels() const;
    int max_hpixels() const;

    int x;
    int y;
    int max_w;
    int max_h;
    std::string scaler;
    uint16_t padding_horizontal;
    uint16_t padding_vertical;

private:
    const Terminal& terminal;

    int width_delta;
    int height_delta;
    int orig_x;
    int orig_y;

    void read_offsets();
};

#endif
