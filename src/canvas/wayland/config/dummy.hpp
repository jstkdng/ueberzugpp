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

#ifndef DUMMY_WAYLAND_CONFIG_H
#define DUMMY_WAYLAND_CONFIG_H

#include "../config.hpp"

class DummyWaylandConfig : public WaylandConfig
{
public:
    DummyWaylandConfig() = default;
    ~DummyWaylandConfig() override = default;
    [[nodiscard]] auto get_window_info() -> struct WaylandWindow override;
    auto is_dummy() -> bool override { return true; }
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;
};

#endif
