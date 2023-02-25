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

#ifndef __X11_CANVAS__
#define __X11_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "window.hpp"
#include "dimensions.hpp"
#include "util/x11.hpp"

#include <xcb/xproto.h>
#include <memory>
#include <vector>

class X11Canvas : public Canvas
{
public:
    X11Canvas();
    ~X11Canvas();

    auto init(const Dimensions& dimensions,
            std::shared_ptr<Image> image) -> void override;
    auto draw() -> void override;
    auto clear() -> void override;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    X11Util xutil;

    std::vector<std::unique_ptr<Window>> windows;
    std::shared_ptr<Image> image;
};

#endif
