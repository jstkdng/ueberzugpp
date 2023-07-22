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

#ifndef WAYLAND_WINDOW_H
#define WAYLAND_WINDOW_H

#include "window.hpp"

#include <memory>
#include <vector>

class WaylandWindow:
    public Window,
    public std::enable_shared_from_this<WaylandWindow>
{
public:
    ~WaylandWindow() override = default;

    virtual void finish_init() = 0;
};

struct XdgStruct
{
    std::weak_ptr<WaylandWindow> ptr;
};

struct XdgStructAgg
{
    std::vector<std::unique_ptr<XdgStruct>> ptrs;
};

#endif
