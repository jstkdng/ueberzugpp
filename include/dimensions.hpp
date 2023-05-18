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
    Dimensions(const Terminal& terminal, uint16_t xcoord, uint16_t ycoord,
            int max_w, int max_h, std::string scaler);

    [[nodiscard]] auto xpixels() const -> uint32_t;
    [[nodiscard]] auto ypixels() const -> uint32_t;
    [[nodiscard]] auto max_wpixels() const -> uint32_t;
    [[nodiscard]] auto max_hpixels() const -> uint32_t;

    uint16_t x;
    uint16_t y;
    uint16_t max_w;
    uint16_t max_h;
    uint16_t padding_horizontal;
    uint16_t padding_vertical;
    std::string scaler;
    const Terminal& terminal;

private:

    uint16_t orig_x;
    uint16_t orig_y;

    void read_offsets();
};

#endif
