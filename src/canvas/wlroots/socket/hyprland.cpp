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

#include "hyprland.hpp"
#include "os.hpp"

#include <fmt/format.h>
#include <iostream>

HyprlandSocket::HyprlandSocket()
{
    const auto env = os::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!env.has_value()) {
        throw std::runtime_error("HYPRLAND NOT SUPPORTED");
    }
    const auto path = fmt::format("/tmp/hypr/{}/.socket.sock", env.value());
    socket = std::make_unique<UnixSocket>(path);
}

auto HyprlandSocket::get_window_info() const -> struct WlrootsWindow
{
    std::cout << "bruh" << std::endl;
    return {.width=0,.height=0,.x=0,.y=0};
}

void HyprlandSocket::disable_focus(std::string_view appid) const
{
    std::cout << appid << std::endl;
}

void HyprlandSocket::enable_floating(std::string_view appid) const
{
    std::cout << appid << std::endl;
}

void HyprlandSocket::move_window(std::string_view appid, int xcoord, int ycoord) const
{
    std::cout << appid << " " << xcoord << " " << ycoord << std::endl;
}
