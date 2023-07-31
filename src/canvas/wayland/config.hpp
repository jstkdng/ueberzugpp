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

#ifndef WAYLAND_CONFIG_H
#define WAYLAND_CONFIG_H

#include <memory>
#include <string_view>

struct WaylandWindowGeometry
{
    int width;
    int height;
    int x;
    int y;
};

class WaylandConfig
{
public:
    static auto get() -> std::unique_ptr<WaylandConfig>;

    virtual ~WaylandConfig() = default;

    virtual auto get_window_info() -> struct WaylandWindowGeometry = 0;
    virtual auto is_dummy() -> bool { return false; }
    virtual void initial_setup(std::string_view appid) = 0;
    virtual void move_window(std::string_view appid, int xcoord, int ycoord) = 0;
};

#endif
