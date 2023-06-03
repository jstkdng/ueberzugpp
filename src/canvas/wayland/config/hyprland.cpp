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
#include "util.hpp"
#include "application.hpp"
#include "tmux.hpp"

#include <fmt/format.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <algorithm>

using njson = nlohmann::json;
using std::begin;
using std::end;

HyprlandSocket::HyprlandSocket()
{
    const auto env = os::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!env.has_value()) {
        throw std::runtime_error("HYPRLAND NOT SUPPORTED");
    }
    logger = spdlog::get("wayland");
    socket_path = fmt::format("/tmp/hypr/{}/.socket.sock", env.value());
    logger->info("Using hyprland socket {}", socket_path);
}

auto HyprlandSocket::request_result(std::string_view payload) -> nlohmann::json
{
    socket = std::make_unique<UnixSocket>(socket_path);
    const int bufsize = 8192;
    std::string result (bufsize, 0);
    socket->write(payload.data(), payload.size());
    socket->read(result.data(), result.size());
    return njson::parse(result);
}

auto HyprlandSocket::get_active_window() -> nlohmann::json
{
    const auto clients = request_result("j/clients");

    std::vector<int> pids {Application::parent_pid_};
    if (tmux::is_used()) {
        const auto tmux_clients = tmux::get_client_pids();
        if (tmux_clients.has_value()) {
            pids = tmux_clients.value();
        }
    }

    for (const auto pid: pids) {
        auto tree = util::get_process_tree(pid);
        for (const auto tpid: tree) {
            const auto client = std::find_if(begin(clients), end(clients), [tpid] (const njson& json) {
                return json["pid"] == tpid;
            });
            if (client != end(clients)) {
                return client.value();
            }
        }
    }
    return {};
}

auto HyprlandSocket::get_active_monitor() -> nlohmann::json
{
    const auto monitors = request_result("j/monitors");
    const auto focused_monitor = std::find_if(begin(monitors), end(monitors), [] (const njson& json) {
        return json["focused"] == true;
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
        .width = sizes[0],
        .height = sizes[1],
        .x = static_cast<int>(coords[0]) + static_cast<int>(monitor.at("x")),
        .y = static_cast<int>(coords[1]) + static_cast<int>(monitor.at("y")),
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
    const auto payload = fmt::format("/keyword windowrulev2 move {} {},title:{}", xcoord, ycoord, appid);
    request(payload);
}

void HyprlandSocket::request(const std::string_view payload)
{
    socket = std::make_unique<UnixSocket>(socket_path);
    logger->debug("Running socket command {}", payload);
    socket->write(payload.data(), payload.length());
}
