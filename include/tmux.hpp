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

#ifndef __NAMESPACE_TMUX__
#define __NAMESPACE_TMUX__

#include <string>
#include <vector>
#include <optional>
#include <string_view>

#include "flags.hpp"

namespace tmux
{
    std::string get_session_id();

    std::string get_pane();

    bool is_used();

    bool is_window_focused();

    auto get_client_pids() -> std::optional<std::vector<int>>;

    auto get_offset() -> std::pair<const int, const int>;

    auto get_pane_offset() -> std::pair<const int, const int>;

    int get_statusbar_offset();

    void handle_hook(std::string_view hook, const Flags& flags);
}

#endif
