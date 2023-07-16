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
#include "tmux.hpp"
#include "util/socket.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

using njson = nlohmann::json;

HyprlandSocket::HyprlandSocket()
{
    const auto env = os::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!env.has_value()) {
        throw std::runtime_error("HYPRLAND NOT SUPPORTED");
    }
    logger = spdlog::get("wayland");
    socket_path = fmt::format("/tmp/hypr/{}/.socket.sock", env.value());
    logger->info("Using hyprland socket {}", socket_path);

    const auto active = request_result("j/activewindow");
    address = active.at("address");
}

auto HyprlandSocket::request_result(const std::string_view payload) -> nlohmann::json
{
    const UnixSocket socket {socket_path};
    socket.write(payload.data(), payload.size());
    const std::string result = socket.read_until_empty();
    return njson::parse(result);
}

void HyprlandSocket::request(const std::string_view payload)
{
    const UnixSocket socket {socket_path};
    logger->debug("Running socket command {}", payload);
    socket.write(payload.data(), payload.length());
}

auto HyprlandSocket::get_active_window() -> nlohmann::json
{
    // recalculate address in case it changed
    if (tmux::is_used()) {
        const auto active = request_result("j/activewindow");
        address = active.at("address");
    }
    const auto clients = request_result("j/clients");
    const auto client = std::ranges::find_if(clients, [this] (const njson& json) {
        return json.at("address") == address;
    });
    if (client == clients.end()) {
        throw std::runtime_error("Active window not found");
    }
    return *client;
}

auto HyprlandSocket::get_active_monitor() -> nlohmann::json
{
    const auto monitors = request_result("j/monitors");
    const auto focused_monitor = std::ranges::find_if(monitors, [] (const njson& json) {
        return json.at("focused") == true;
    });
    return focused_monitor.value();
}

auto HyprlandSocket::get_window_info() -> struct WaylandWindow
{
    const auto terminal = get_active_window();
    const auto monitor = get_active_monitor();
    const auto& sizes = terminal.at("size");
    const auto& coords = terminal.at("at");

    return {
        .width = sizes.at(0),
        .height = sizes.at(1),
        .x = coords.at(0).get<int>() - monitor.at("x").get<int>(),
        .y = coords.at(1).get<int>() - monitor.at("y").get<int>(),
    };
}

void HyprlandSocket::initial_setup(const std::string_view appid)
{
    disable_focus(appid);
    enable_floating(appid);
    remove_borders(appid);
    remove_rounding(appid);
}

void HyprlandSocket::remove_rounding(const std::string_view appid)
{
    const auto payload = fmt::format("/keyword windowrulev2 rounding 0,title:{}", appid);
    request(payload);
}

void HyprlandSocket::disable_focus(const std::string_view appid)
{
    const auto payload = fmt::format("/keyword windowrulev2 nofocus,title:{}", appid);
    request(payload);
}

void HyprlandSocket::enable_floating(const std::string_view appid)
{
    const auto payload = fmt::format("/keyword windowrulev2 float,title:{}", appid);
    request(payload);
}

void HyprlandSocket::remove_borders(const std::string_view appid)
{
    const auto payload = fmt::format("/keyword windowrulev2 noborder,title:{}", appid);
    request(payload);
}

void HyprlandSocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    const auto payload = fmt::format("/dispatch movewindowpixel exact {} {},title:{}", xcoord, ycoord, appid);
    request(payload);
}
