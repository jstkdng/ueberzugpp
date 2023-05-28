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

#ifndef __SWAY_SOCKET__
#define __SWAY_SOCKET__

#include "util/socket.hpp"
#include "../socket.hpp"

#include <string_view>
#include <memory>
#include <nlohmann/json.hpp>

enum ipc_message_type {
    IPC_COMMAND = 0,
    IPC_GET_WORKSPACES = 1,
    IPC_GET_TREE = 4
};

class SwaySocket : public WlrootsSocket
{
public:
    SwaySocket();
    ~SwaySocket() override = default;
    [[nodiscard]] auto get_window_info() -> struct WlrootsWindow override;
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;

private:
    std::unique_ptr<UnixSocket> socket;

    void disable_focus(std::string_view appid);
    void enable_floating(std::string_view appid);
    [[nodiscard]] auto current_window() const -> nlohmann::json;
    [[nodiscard]] auto ipc_command(std::string_view appid, std::string_view command) const -> nlohmann::json;
    [[nodiscard]] auto ipc_command(std::string_view command) const -> nlohmann::json;
    [[nodiscard]] auto ipc_message(ipc_message_type type, std::string_view payload = "") const -> nlohmann::json;
};

#endif
