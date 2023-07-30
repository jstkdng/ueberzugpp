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

#include "wayfire.hpp"

#include <spdlog/spdlog.h>

WayfireSocket::WayfireSocket(const std::string_view endpoint):
socket(endpoint),
logger(spdlog::get("wayland"))
{
    logger->info("Using wayfire socket {}", endpoint);
}

auto WayfireSocket::get_window_info() -> struct WaylandWindowGeometry
{
    return {};
}

void WayfireSocket::initial_setup([[maybe_unused]] const std::string_view appid)
{}

void WayfireSocket::move_window([[maybe_unused]] const std::string_view appid,
        [[maybe_unused]] int xcoord, [[maybe_unused]] int ycoord)
{
    // all is handled by the ueberzug plugin
}
