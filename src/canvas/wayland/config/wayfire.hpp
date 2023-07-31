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

#ifndef WAYFIRE_SOCKET_H
#define WAYFIRE_SOCKET_H

#include "util/socket.hpp"
#include "../config.hpp"

#include <spdlog/fwd.h>
#include <nlohmann/json.hpp>

class WayfireSocket : public WaylandConfig
{
public:
    explicit WayfireSocket(std::string_view endpoint);
    ~WayfireSocket() override = default;

    auto get_window_info() -> struct WaylandWindowGeometry override;
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;

private:
    [[nodiscard]] auto request(std::string_view method, const nlohmann::json& data = {}) const -> nlohmann::json;

    const UnixSocket socket;
    std::shared_ptr<spdlog::logger> logger;
};

#endif
