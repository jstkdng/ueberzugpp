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
#include <nlohmann/json.hpp>

using njson = nlohmann::json;

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
{
    // all is handled by the ueberzug plugin
}

void WayfireSocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    const njson json = {
        {"method", "ueberzugpp/set_offset"},
        {"data", {
            {"app-id", appid},
            {"x", xcoord},
            {"y", ycoord}
        }}
    };
    const auto data = json.dump();
    const uint32_t data_size = data.length();
    socket.write(&data_size, sizeof(uint32_t));
    socket.write(data.c_str(), data_size);
}
