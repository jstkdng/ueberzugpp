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

#include "dimensions.hpp"
#include "tmux.hpp"

Dimensions::Dimensions(const Terminal& terminal, int x, int y, int max_w, int max_h):
terminal(terminal),
x(x), y(y), max_w(max_w), max_h(max_h)
{
    auto [offset_x, offset_y] = tmux::get_offset();
    this->x += offset_x;
    this->y += offset_y;
}

int Dimensions::xpixels() const
{
    return x * terminal.font_width;
}

int Dimensions::ypixels() const
{
    return y * terminal.font_height;
}

int Dimensions::max_wpixels() const
{
    return max_w * terminal.font_width;
}

int Dimensions::max_hpixels() const
{
    return max_h * terminal.font_height;
}
