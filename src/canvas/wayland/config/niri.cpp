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

#include "niri.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <tuple>

#include <fmt/format.h>
#include <range/v3/all.hpp>

using njson = nlohmann::json;

namespace
{

constexpr auto ueberzug_appid_prefix = std::string_view{"ueberzugpp_"};

auto string_value(const njson &json, const std::string_view key) -> std::string
{
    const auto iter = json.find(std::string{key});
    if (iter == json.end() || iter->is_null()) {
        return {};
    }
    return iter->get<std::string>();
}

auto is_ueberzug_window(const njson &window) -> bool
{
    const auto appid = string_value(window, "app_id");
    const auto title = string_value(window, "title");
    return appid.starts_with(ueberzug_appid_prefix) || title.starts_with(ueberzug_appid_prefix);
}

auto json_window_id(const njson &window) -> uint64_t
{
    return window.at("id").get<uint64_t>();
}

auto position_change(double value) -> njson
{
    return {{"SetFixed", value}};
}

auto scaled_coord(double value, float scale) -> int
{
    return static_cast<int>(std::lround(value * scale));
}

auto read_niri_response(const UnixSocket &socket) -> std::string
{
    std::string response;
    char readch = '\0';

    while (true) {
        readch = '\0';
        socket.read(&readch, sizeof(readch));
        if (readch == '\0' || readch == '\n') {
            break;
        }
        response.push_back(readch);
    }
    return response;
}

} // namespace

NiriSocket::NiriSocket(const std::string_view endpoint)
    : socket_path(endpoint),
      logger(spdlog::get("wayland"))
{
    logger->info("Using niri socket {}", endpoint);
    set_active_output_info();
    refresh_terminal_window();
}

auto NiriSocket::get_focused_output_name() -> std::string
{
    if (output_info.name.empty()) {
        set_active_output_info();
    }
    return output_info.name;
}

auto NiriSocket::get_window_info() -> struct WaylandWindowGeometry {
    const auto window = get_terminal_window();
    const auto &layout = window.at("layout");
    const auto &window_size = layout.at("window_size");

    double xpos = 0;
    double ypos = 0;
    const auto tile_pos_iter = layout.find("tile_pos_in_workspace_view");
    if (tile_pos_iter != layout.end() && !tile_pos_iter->is_null()) {
        const auto &tile_pos = *tile_pos_iter;
        xpos = tile_pos.at(0).get<double>();
        ypos = tile_pos.at(1).get<double>();
    }

    const auto &window_offset = layout.at("window_offset_in_tile");
    xpos += window_offset.at(0).get<double>();
    ypos += window_offset.at(1).get<double>();

    return {
        .width = scaled_coord(window_size.at(0).get<double>(), output_info.scale),
        .height = scaled_coord(window_size.at(1).get<double>(), output_info.scale),
        .x = scaled_coord(xpos, output_info.scale),
        .y = scaled_coord(ypos, output_info.scale),
    };
}

void NiriSocket::initial_setup([[maybe_unused]] const std::string_view appid)
{
    set_active_output_info();
    refresh_terminal_window();
}

void NiriSocket::move_window(const std::string_view appid, int xcoord, int ycoord)
{
    const auto window = find_window(appid);
    if (!window.has_value()) {
        logger->debug("Could not find niri window for app id {}", appid);
        return;
    }

    const auto id = json_window_id(window.value());
    ensure_window_setup(id);

    double res_x = xcoord;
    double res_y = ycoord;
    if (output_info.scale > 1.0F) {
        res_x /= output_info.scale;
        res_y /= output_info.scale;
    }

    const njson payload = {
        {"MoveFloatingWindow", {{"id", id}, {"x", position_change(res_x)}, {"y", position_change(res_y)}}}};
    action(payload);

    if (terminal_window_id.has_value()) {
        focus_window(terminal_window_id.value());
    }
}

auto NiriSocket::request(const njson &payload) const -> njson
{
    const UnixSocket socket{socket_path};
    const auto request_payload = fmt::format("{}\n", payload.dump());
    socket.write(request_payload.data(), request_payload.size());

    const auto response = njson::parse(read_niri_response(socket));
    if (response.contains("Err")) {
        throw std::runtime_error(fmt::format("niri IPC error: {}", response.at("Err").dump()));
    }
    return response.at("Ok");
}

auto NiriSocket::focused_output() const -> njson
{
    return request("FocusedOutput").at("FocusedOutput");
}

auto NiriSocket::focused_window() const -> njson
{
    return request("FocusedWindow").at("FocusedWindow");
}

auto NiriSocket::get_windows() const -> njson
{
    return request("Windows").at("Windows");
}

auto NiriSocket::find_window(const std::string_view appid) const -> std::optional<njson>
{
    const std::string appid_str{appid};
    constexpr auto attempts = 20;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        const auto windows = get_windows();
        const auto found = ranges::find_if(windows, [&appid_str](const njson &window) {
            return string_value(window, "app_id") == appid_str || string_value(window, "title") == appid_str;
        });
        if (found != windows.end()) {
            return std::make_optional<njson>(*found);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    return {};
}

auto NiriSocket::get_terminal_window() -> njson
{
    const auto windows = get_windows();
    if (terminal_window_id.has_value()) {
        const auto found = ranges::find_if(
            windows, [this](const njson &window) { return json_window_id(window) == terminal_window_id.value(); });
        if (found != windows.end()) {
            return *found;
        }
        terminal_window_id.reset();
    }

    const auto window = focused_window();
    if (!window.is_null() && !is_ueberzug_window(window)) {
        terminal_window_id = json_window_id(window);
        return window;
    }

    throw std::runtime_error("Could not find focused niri terminal window");
}

void NiriSocket::action(const njson &payload) const
{
    std::ignore = request({{"Action", payload}});
}

void NiriSocket::ensure_window_setup(WindowId id)
{
    if (initialized_windows.contains(id)) {
        return;
    }

    action({{"MoveWindowToFloating", {{"id", id}}}});
    initialized_windows.insert(id);
}

void NiriSocket::focus_window(WindowId id) const
{
    action({{"FocusWindow", {{"id", id}}}});
}

void NiriSocket::refresh_terminal_window()
{
    const auto window = focused_window();
    if (window.is_null() || is_ueberzug_window(window)) {
        return;
    }
    terminal_window_id = json_window_id(window);
}

void NiriSocket::set_active_output_info()
{
    const auto output = focused_output();
    if (output.is_null()) {
        throw std::runtime_error("Could not find focused niri output");
    }

    const auto &logical = output.at("logical");
    output_info = {
        .x = logical.at("x").get<int>(),
        .y = logical.at("y").get<int>(),
        .scale = logical.at("scale").get<float>(),
        .name = output.at("name").get<std::string>(),
    };
}

auto NiriSocket::get_output_scale() -> std::optional<float>
{
    return output_info.scale;
}
