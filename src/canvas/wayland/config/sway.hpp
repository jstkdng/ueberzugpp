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

#ifndef SWAY_SOCKET_H
#define SWAY_SOCKET_H

#include "util/socket.hpp"
#include "../config.hpp"

#include <string_view>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/fwd.h>

enum ipc_message_type {
    IPC_COMMAND = 0,
    IPC_GET_WORKSPACES = 1,
    IPC_GET_TREE = 4
};

class SwaySocket : public WaylandConfig
{
public:
    SwaySocket();
    ~SwaySocket() override = default;
    [[nodiscard]] auto get_window_info() -> struct WaylandWindowGeometry override;
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;

private:
    std::unique_ptr<UnixSocket> socket;
    std::shared_ptr<spdlog::logger> logger;
    std::string socket_path;

    void disable_focus(std::string_view appid);
    void enable_floating(std::string_view appid);
    void ipc_command(std::string_view appid, std::string_view command) const;
    void ipc_command(std::string_view payload) const;
    [[nodiscard]] auto get_nodes() const -> std::vector<nlohmann::json>;
    [[nodiscard]] auto ipc_message(ipc_message_type type, std::string_view payload = "") const -> nlohmann::json;
    [[nodiscard]] static auto get_active_window(const std::vector<nlohmann::json>& nodes) -> nlohmann::json;
    [[nodiscard]] static auto get_active_monitor(const std::vector<nlohmann::json>& nodes) -> nlohmann::json;
};

#endif
