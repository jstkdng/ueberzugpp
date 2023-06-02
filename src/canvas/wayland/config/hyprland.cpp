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
#include <execution>
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
}

auto HyprlandSocket::get_window_info() -> struct WaylandWindow
{
    using std::begin, std::end;
    socket = std::make_unique<UnixSocket>(socket_path);
    const std::string_view payload {"j/clients"};
    const int bufsize = 8192;
    std::string result (bufsize, 0);
    socket->write(payload.data(), payload.size());
    socket->read(result.data(), result.size());
    const auto clients = njson::parse(result);

    std::vector<int> pids {Application::parent_pid_};
    if (tmux::is_used()) {
        const auto tmux_clients = tmux::get_client_pids();
        if (tmux_clients.has_value()) {
            pids = tmux_clients.value();
        }
    }

    struct WaylandWindow window;
    std::for_each(std::execution::par_unseq, begin(pids), end(pids), [&] (int pid) {
        const auto tree = util::get_process_tree(pid);
        const auto client = std::find_if(std::execution::par_unseq, begin(clients), end(clients), [&] (const njson& json) {
            return std::find(std::execution::par_unseq, begin(tree), end(tree), json["pid"]) != end(tree);
        });
        if (client != end(clients)) {
            const auto& sizes = client->at("size");
            const auto& coords = client->at("at");
            window.width = sizes[0];
            window.height = sizes[1];
            window.x = coords[0];
            window.y = coords[1];
        }
    });
    return window;
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
