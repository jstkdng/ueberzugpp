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

#ifndef __X11_UTIL__
#define __X11_UTIL__

#include <xcb/xproto.h>
#include <vector>
#include <unordered_map>

class X11Util
{
public:
    X11Util();
    ~X11Util();

    [[nodiscard]] auto get_server_window_ids() const -> std::vector<xcb_window_t>;
    [[nodiscard]] auto get_pid_window_map() const -> std::unordered_map<int, xcb_window_t>;
    [[nodiscard]] auto get_window_dimensions(xcb_window_t window) const -> std::pair<int, int>;
    [[nodiscard]] auto get_parent_window(int pid) const -> xcb_window_t;

    [[nodiscard]] auto window_has_property(xcb_window_t window, xcb_atom_t property, xcb_atom_t type = XCB_ATOM_ANY) const -> bool;
    [[nodiscard]] auto get_window_pid(xcb_window_t window) const -> int;

    bool connected = false;
private:
    xcb_connection_t* connection = nullptr;
    xcb_screen_t* screen = nullptr;

    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows,
        xcb_query_tree_cookie_t cookie) const -> void;
};

#endif
