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

#ifndef __WLROOTS_SOCKET__
#define __WLROOTS_SOCKET__

#include <memory>
#include <string_view>

struct WlrootsWindow
{
    int width;
    int height;
    int x;
    int y;
};

class WlrootsSocket
{
public:
    static auto get() -> std::unique_ptr<WlrootsSocket>;

    virtual ~WlrootsSocket() = default;

    [[nodiscard]] virtual auto get_window_info() const -> struct WlrootsWindow = 0;
    virtual void disable_focus(std::string_view appid) const = 0;
    virtual void enable_floating(std::string_view appid) const = 0;
    virtual void move_window(std::string_view appid, int xcoord, int ycoord) const = 0;
};

#endif
