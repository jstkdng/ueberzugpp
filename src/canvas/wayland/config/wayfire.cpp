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

using njson = nlohmann::json;

WayfireSocket::WayfireSocket(const std::string_view endpoint):
socket(endpoint),
logger(spdlog::get("wayland"))
{
    logger->info("Using wayfire socket {}", endpoint);
}

auto WayfireSocket::get_window_info() -> struct WaylandWindowGeometry
{
    const auto response = request("window-rules/get-focused-view");
    const auto& info = response.at("info");
    const auto& geometry = info.at("geometry");
    const int decoration_height = 25;
    return {
        .width = geometry.at("width"),
        .height = geometry.at("height").get<int>() - decoration_height,
        .x = 0,
        .y = 0
    };
}

void WayfireSocket::initial_setup([[maybe_unused]] const std::string_view appid)
{
    // all is handled by the ueberzug plugin
}

void WayfireSocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    const njson payload_data = {
        {"app-id", appid},
        {"x", xcoord},
        {"y", ycoord}
    };
    std::ignore = request("ueberzugpp/set_offset", payload_data);
}

auto WayfireSocket::request(const std::string_view method, const njson& data) const -> njson
{
    const njson json = {
        {"method", method},
        {"data", data}
    };
    const auto payload = json.dump();
    const uint32_t payload_size = payload.length();
    socket.write(&payload_size, sizeof(uint32_t));
    socket.write(payload.c_str(), payload_size);

    uint32_t response_size = 0;
    socket.read(&response_size, sizeof(uint32_t));
    std::string buffer (response_size, 0);
    socket.read(buffer.data(), response_size);
    return njson::parse(buffer);
}
