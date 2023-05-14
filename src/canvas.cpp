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

#include "canvas.hpp"
#include "canvas/sixel.hpp"
#include "canvas/kitty/kitty.hpp"
#include "canvas/iterm2/iterm2.hpp"
#ifdef ENABLE_X11
#   include "canvas/x11/x11.hpp"
#endif
#include "flags.hpp"

auto Canvas::create(std::mutex& img_lock) -> std::unique_ptr<Canvas>
{
    auto flags = Flags::instance();
    if (flags->output == "kitty") {
        return std::make_unique<KittyCanvas>();
    }
#ifdef ENABLE_X11
    if (flags->output == "x11") {
        return std::make_unique<X11Canvas>(img_lock);
    }
#endif
    if (flags->output == "iterm2") {
        return std::make_unique<Iterm2Canvas>();
    }
    if (flags->output == "sixel") {
        return std::make_unique<SixelCanvas>(img_lock);
    }
    return nullptr;
}
