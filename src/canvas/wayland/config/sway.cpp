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

#include "sway.hpp"
#include "os.hpp"
#include "application.hpp"
#include "tmux.hpp"
#include "util.hpp"

#include <string>
#include <array>
#include <stack>
#include <algorithm>
#include <gsl/gsl>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

using njson = nlohmann::json;

constexpr auto ipc_magic = std::string_view{"i3-ipc"};
constexpr auto ipc_header_size = ipc_magic.size() + 8;

SwaySocket::SwaySocket(const std::string_view endpoint):
socket(endpoint),
logger(spdlog::get("wayland"))
{
    logger->info("Using sway socket {}", endpoint);
}

struct __attribute__((packed)) ipc_header {
    std::array<char, ipc_magic.size()> magic;
    uint32_t len;
    uint32_t type;
};

auto SwaySocket::get_window_info() -> struct WaylandWindowGeometry
{
    const auto nodes = get_nodes();
    const auto window = get_active_window(nodes);
    const auto monitor = get_active_monitor(nodes);
    const auto& mon_rect = monitor.at("rect");
    const auto& rect = window.at("rect");
    return {
        .width = rect.at("width"),
        .height = rect.at("height"),
        .x = rect.at("x").get<int>() - mon_rect.at("x").get<int>(),
        .y = rect.at("y").get<int>() - mon_rect.at("y").get<int>()
    };
}

auto SwaySocket::get_active_window(const std::vector<nlohmann::json>& nodes) -> nlohmann::json
{
    const auto pids = tmux::get_client_pids().value_or(std::vector<int>{Application::parent_pid_});

    for (const auto pid: pids) {
        const auto tree = util::get_process_tree(pid);
        const auto found = std::ranges::find_if(nodes, [&tree] (const njson& json) -> bool {
            try {
                return std::ranges::find(tree, json.at("pid").get<int>()) != tree.end();
            } catch (const njson::out_of_range& err) {
                return false;
            }
        });
        if (found != nodes.end()) {
            return *found;
        }
    }
    return nullptr;
}

auto SwaySocket::get_active_monitor(const std::vector<nlohmann::json>& nodes) -> nlohmann::json
{
    const int focus_id = nodes.at(0).at("focus").at(0);
    for (const auto& node: nodes) {
        if (node.at("id") == focus_id) {
            scale_factor = node.at("scale");
            return node;
        }
    }
    return nullptr;
}

void SwaySocket::initial_setup(const std::string_view appid)
{
    disable_focus(appid);
    enable_floating(appid);
}

void SwaySocket::disable_focus(const std::string_view appid)
{
    const auto payload = fmt::format(R"(no_focus [app_id="{}"])", appid);
    ipc_command(payload);
}

void SwaySocket::enable_floating(const std::string_view appid)
{
    ipc_command(appid, "floating enable");
}

void SwaySocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    int res_x = xcoord;
    int res_y = ycoord;
    if (scale_factor > 1.0F) {
        const float scale_magic = 0.5;
        res_x = std::floor(gsl::narrow_cast<float>(res_x) * scale_magic);
        res_y = std::floor(gsl::narrow_cast<float>(res_y) * scale_magic);
    }
    const auto payload = fmt::format(R"([app_id="{}"] move position {} {})", appid, res_x, res_y);
    ipc_command(payload);
}

auto SwaySocket::ipc_message(ipc_message_type type, const std::string_view payload) const -> nlohmann::json
{
    struct ipc_header header;
    header.len = payload.size();
    header.type = type;
    ipc_magic.copy(header.magic.data(), ipc_magic.size());

    if (!payload.empty()) {
        logger->debug("Running socket command {}", payload);
    }
    socket.write(&header, ipc_header_size);
    socket.write(payload.data(), payload.size());

    socket.read(&header, ipc_header_size);
    std::string buff (header.len, 0);
    socket.read(buff.data(), buff.size());
    return njson::parse(buff);
}

auto SwaySocket::get_nodes() const -> std::vector<nlohmann::json>
{
    logger->debug("Obtaining sway tree");
    const auto tree = ipc_message(IPC_GET_TREE);
    std::stack<njson> nodes_st;
    std::vector<njson> nodes_vec;

    nodes_st.push(tree);

    while (!nodes_st.empty()) {
        const auto top = nodes_st.top();
        nodes_st.pop();
        nodes_vec.push_back(top);
        for (const auto& node: top.at("nodes")) {
            nodes_st.push(node);
        }
        for (const auto& node: top.at("floating_nodes")) {
            nodes_st.push(node);
        }
    }
    return nodes_vec;
}

void SwaySocket::ipc_command(const std::string_view appid, const std::string_view command) const
{
    const auto payload = fmt::format(R"(for_window [app_id="{}"] {})", appid, command);
    ipc_command(payload);
}

void SwaySocket::ipc_command(const std::string_view payload) const
{
    std::ignore = ipc_message(IPC_COMMAND, payload);
}
