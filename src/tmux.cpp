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

#include "tmux.hpp"
#include "os.hpp"
#include "util.hpp"

#include <string>
#include <array>
#include <fmt/format.h>

constexpr auto hooks = std::to_array<std::string_view>({
    "client-session-changed",
    "session-window-changed",
    "client-detached",
    "window-layout-changed"
});

auto tmux::get_session_id() -> std::string
{
    const auto cmd = fmt::format("tmux display -p -F '#{{session_id}}' -t {}", tmux::get_pane());
    return os::exec(cmd);
}

auto tmux::is_used() -> bool
{
    return !tmux::get_pane().empty();
}

auto tmux::is_window_focused() -> bool
{
    const auto cmd = fmt::format("tmux display -p -F '#{{window_active}},#{{pane_in_mode}}' -t {}", tmux::get_pane());
    return os::exec(cmd) == "1,0";
}

auto tmux::get_pane() -> std::string
{
    return os::getenv("TMUX_PANE").value_or("");
}

auto tmux::get_client_pids() -> std::optional<std::vector<int>>
{
    if (!tmux::is_used()) {
        return {};
    }
    if (!tmux::is_window_focused()) {
        return {};
    }

    std::vector<int> pids;
    const auto cmd = fmt::format("tmux list-clients -F '#{{client_pid}}' -t {}", tmux::get_pane());
    const auto output = os::exec(cmd);

    for (const auto& line: util::str_split(output, "\n")) {
        pids.push_back(std::stoi(line));
    }

    return pids;
}

auto tmux::get_offset() -> std::pair<int, int>
{
    if (!tmux::is_used()) {
        return std::make_pair(0, 0);
    }
    const auto [p_x, p_y] = get_pane_offset();
    const auto s_y = get_statusbar_offset();
    return std::make_pair(p_x, p_y + s_y);
}

auto tmux::get_pane_offset() -> std::pair<int, int>
{
    const std::string cmd = "tmux display -p -F '#{pane_top},#{pane_left},\
                                     #{pane_bottom},#{pane_right},\
                                     #{window_height},#{window_width}' \
                                  -t" +  tmux::get_pane();
    const auto output = util::str_split(os::exec(cmd), ",");
    return std::make_pair(std::stoi(output[1]), std::stoi(output[0]));
}

auto tmux::get_statusbar_offset() -> int
{
    const std::string cmd = "tmux display -p '#{status},#{status-position}'";
    const auto output = util::str_split(os::exec(cmd), ",");
    if (output[1] != "top" || output[0] == "off") {
        return 0;
    }
    if (output[0] == "on") {
        return 1;
    }
    return std::stoi(output[0]);
}

void tmux::handle_hook(const std::string_view hook, int pid)
{
    const auto msg = fmt::format(R"({{"action":"tmux","hook":"{}"}})", hook);
    const auto endpoint = util::get_socket_endpoint(pid);
    util::send_socket_message(msg, endpoint);
}

void tmux::register_hooks()
{
    if (!is_used()) {
        return;
    }
    for (const auto& hook: hooks) {
        const auto cmd = fmt::format(
                R"(tmux set-hook -t {} {} "run-shell 'ueberzug tmux {} {}'")",
                get_pane(), hook, hook, os::get_pid());
        os::exec(cmd);
    }
}

void tmux::unregister_hooks()
{
    if (!is_used()) {
        return;
    }
    for (const auto& hook: hooks) {
        const auto cmd = fmt::format(
                "tmux set-hook -u -t {} {}", get_pane(), hook);
        os::exec(cmd);
    }
}
