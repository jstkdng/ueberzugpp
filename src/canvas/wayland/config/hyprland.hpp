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

#ifndef __HYPRLAND_SOCKET__
#define __HYPRLAND_SOCKET__

#include "util/socket.hpp"
#include "../config.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

class HyprlandSocket : public WaylandConfig
{
public:
    HyprlandSocket();
    ~HyprlandSocket() override = default;
    [[nodiscard]] auto get_window_info() -> struct WaylandWindow override;
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;

private:
    void disable_focus(std::string_view appid);
    void enable_floating(std::string_view appid);
    void remove_borders(std::string_view appid);
    void remove_rounding(std::string_view appid);
    void request(std::string_view payload);
    auto request_result(std::string_view payload) -> nlohmann::json;
    auto get_active_window() -> nlohmann::json;
    auto get_active_monitor() -> nlohmann::json;

    std::unique_ptr<UnixSocket> socket;
    std::shared_ptr<spdlog::logger> logger;
    std::string socket_path;
    std::string address;
};

#endif
