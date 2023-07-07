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

#include <fmt/format.h>

using njson = nlohmann::json;

constexpr auto ipc_magic = std::string_view{"i3-ipc"};
constexpr auto ipc_header_size = ipc_magic.size() + 8;

SwaySocket::SwaySocket()
{
    const auto sway_sock = os::getenv("SWAYSOCK");
    if (!sway_sock.has_value()) {
        throw std::runtime_error("SWAY NOT SUPPORTED");
    }
    socket_path = sway_sock.value();
    logger = spdlog::get("wayland");
    socket = std::make_unique<UnixSocket>(socket_path);
    logger->info("Using sway socket {}", socket_path);
}

struct __attribute__((packed)) ipc_header {
    std::array<char, ipc_magic.size()> magic;
    uint32_t len;
    uint32_t type;
};

auto SwaySocket::get_window_info() -> struct WaylandWindow
{
    const auto nodes = get_nodes();
    const auto window = get_active_window(nodes);
    const auto monitor = get_active_monitor(nodes);
    const auto& mon_rect = monitor["rect"];
    const auto& rect = window["rect"];
    return {
        .width = rect["width"],
        .height = rect["height"],
        .x = static_cast<int>(rect["x"]) - static_cast<int>(mon_rect["x"]),
        .y = static_cast<int>(rect["y"]) - static_cast<int>(mon_rect["y"])
    };
}

auto SwaySocket::get_active_window(const std::vector<nlohmann::json>& nodes) -> nlohmann::json
{
    const auto pids = tmux::get_client_pids().value_or(std::vector<int>{Application::parent_pid_});

    for (const auto pid: pids) {
        const auto tree = util::get_process_tree(pid);
        const auto found = std::find_if(nodes.begin(), nodes.end(), [&tree] (const njson& json) -> bool {
            try {
                return std::find(tree.begin(), tree.end(), json.at("pid")) != tree.end();
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
    std::ignore = ipc_command(fmt::format(R"(no_focus [app_id="{}"])", appid));
}

void SwaySocket::enable_floating(const std::string_view appid)
{
    std::ignore = ipc_command(appid, "floating enable");
}

void SwaySocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    std::ignore = ipc_command(fmt::format(R"([app_id="{}"] move absolute position {} {})", appid, xcoord, ycoord));
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
    socket->write(reinterpret_cast<const void*>(&header), ipc_header_size);
    socket->write(reinterpret_cast<const void*>(payload.data()), payload.size());

    socket->read(reinterpret_cast<void*>(&header), ipc_header_size);
    std::string buff (header.len, 0);
    socket->read(reinterpret_cast<void*>(buff.data()), buff.size());
    return njson::parse(buff);
}

auto SwaySocket::get_nodes() const -> std::vector<nlohmann::json>
{
    logger->debug("Obtaining sway tree");
    auto tree = ipc_message(IPC_GET_TREE);
    std::stack<njson> nodes_st;
    std::vector<njson> nodes_vec;

    nodes_st.push(tree);

    while (!nodes_st.empty()) {
        auto top = nodes_st.top();
        nodes_st.pop();
        nodes_vec.push_back(top);
        for (auto& node: top["nodes"]) {
            nodes_st.push(node);
        }
        for (auto& node: top["floating_nodes"]) {
            nodes_st.push(node);
        }
    }
    return nodes_vec;
}

auto SwaySocket::ipc_command(std::string_view appid, std::string_view command) const -> nlohmann::json
{
    const auto cmd = fmt::format(R"(for_window [app_id="{}"] {})", appid, command);
    return ipc_message(IPC_COMMAND, cmd);
}

auto SwaySocket::ipc_command(std::string_view command) const -> nlohmann::json
{
    return ipc_message(IPC_COMMAND, command);
}
