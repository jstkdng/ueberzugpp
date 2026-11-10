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

#ifndef NIRI_SOCKET_H
#define NIRI_SOCKET_H

#include "../config.hpp"
#include "util/socket.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

struct NiriOutputInfo {
    int x = 0;
    int y = 0;
    float scale = 1.0F;
    std::string name;
};

class NiriSocket : public WaylandConfig
{
  public:
    explicit NiriSocket(std::string_view endpoint);
    ~NiriSocket() override = default;

    auto get_focused_output_name() -> std::string override;
    auto get_output_scale() -> std::optional<float> override;
    auto get_window_info() -> struct WaylandWindowGeometry override;
    void initial_setup(std::string_view appid) override;
    void move_window(std::string_view appid, int xcoord, int ycoord) override;

  private:
    using WindowId = uint64_t;

    [[nodiscard]] auto request(const nlohmann::json &payload) const -> nlohmann::json;
    [[nodiscard]] auto focused_output() const -> nlohmann::json;
    [[nodiscard]] auto focused_window() const -> nlohmann::json;
    [[nodiscard]] auto get_windows() const -> nlohmann::json;
    [[nodiscard]] auto find_window(std::string_view appid) const -> std::optional<nlohmann::json>;
    [[nodiscard]] auto get_terminal_window() -> nlohmann::json;

    void action(const nlohmann::json &payload) const;
    void ensure_window_setup(WindowId id);
    void focus_window(WindowId id) const;
    void refresh_terminal_window();
    void set_active_output_info();

    std::string socket_path;
    std::shared_ptr<spdlog::logger> logger;
    struct NiriOutputInfo output_info;
    std::optional<WindowId> terminal_window_id;
    std::unordered_set<WindowId> initialized_windows;
};

#endif
