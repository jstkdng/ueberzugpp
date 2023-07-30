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

#include "config.hpp"
#include "os.hpp"
#include "config/sway.hpp"
#include "config/hyprland.hpp"
#include "config/wayfire.hpp"
#include "config/dummy.hpp"

auto WaylandConfig::get() -> std::unique_ptr<WaylandConfig>
{
    const auto sway_sock = os::getenv("SWAYSOCK");
    if (sway_sock.has_value()) {
        return std::make_unique<SwaySocket>();
    }

    const auto hypr_sig = os::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (hypr_sig.has_value()) {
        return std::make_unique<HyprlandSocket>();
    }

    const auto wayfire_sock = os::getenv("WAYFIRE_SOCKET");
    if (wayfire_sock.has_value()) {
        return std::make_unique<WayfireSocket>(wayfire_sock.value());
    }

    return std::make_unique<DummyWaylandConfig>();
}

