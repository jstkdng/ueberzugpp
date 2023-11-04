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
#include "terminal.hpp"
#include "flags.hpp"

#include <utility>
#include <gsl/gsl>
#include <iostream>

Dimensions::Dimensions(const Terminal* terminal, uint16_t xcoord,
        uint16_t ycoord, int max_w, int max_h, std::string scaler):
max_w(max_w),
max_h(max_h),
padding_horizontal(terminal->padding_horizontal),
padding_vertical(terminal->padding_vertical),
scaler(std::move(scaler)),
terminal(terminal),
orig_x(xcoord),
orig_y(ycoord),
flags(Flags::instance())
{
    read_offsets();
}

void Dimensions::read_offsets()
{
    const auto [offset_x, offset_y] = tmux::get_offset();
    x = orig_x + offset_x;
    y = orig_y + offset_y;
}

auto Dimensions::xpixels() const -> int
{
    return gsl::narrow_cast<int>(gsl::narrow_cast<float>(x * terminal->font_width) * flags->screen_scale);
}

auto Dimensions::ypixels() const -> int
{
    return gsl::narrow_cast<int>(gsl::narrow_cast<float>(y * terminal->font_height) * flags->screen_scale);
}

auto Dimensions::max_wpixels() const -> int
{
    return gsl::narrow_cast<int>(gsl::narrow_cast<float>(max_w * terminal->font_width) * flags->screen_scale);
}

auto Dimensions::max_hpixels() const -> int
{
    return gsl::narrow_cast<int>(gsl::narrow_cast<float>(max_h * terminal->font_height) * flags->screen_scale);
}
