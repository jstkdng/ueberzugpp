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

#include "util/x11.hpp"
#include "util.hpp"
#include "os.hpp"
#include "util/ptr.hpp"

#include <xcb/xcb.h>
#include <string>
#include <iostream>

X11Util::X11Util():
connection(xcb_connect(nullptr, nullptr))
{
    if (xcb_connection_has_error(connection) == 0) {
        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
        connected = true;
    }
}

X11Util::~X11Util()
{
    xcb_disconnect(connection);
}

auto X11Util::get_server_window_ids() const -> std::vector<xcb_window_t>
{
    auto cookie = xcb_query_tree_unchecked(connection, screen->root);
    std::vector<xcb_window_t> windows;
    get_server_window_ids_helper(windows, cookie);
    return windows;
}

void X11Util::get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) const
{
    auto reply = unique_C_ptr<xcb_query_tree_reply_t> {
        xcb_query_tree_reply(connection, cookie, nullptr)
    };
    if (reply.get() == nullptr) {
        throw std::runtime_error("UNABLE TO QUERY WINDOW TREE");
    }
    int num_children = xcb_query_tree_children_length(reply.get());

    if (num_children == 0) {
        return;
    }

    auto *children = xcb_query_tree_children(reply.get());
    std::vector<xcb_query_tree_cookie_t> cookies;

    for (int i = 0; i < num_children; ++i) {
        auto child = children[i];
        windows.push_back(child);
        cookies.push_back(xcb_query_tree_unchecked(connection, child));
    }

    for (auto new_cookie: cookies) {
        get_server_window_ids_helper(windows, new_cookie);
    }
}

auto X11Util::get_window_pid(xcb_window_t window) const -> int
{
    std::string atom_str = "_NET_WM_PID";

    auto atom_cookie = xcb_intern_atom_unchecked
        (connection, 0, atom_str.size(), atom_str.c_str());
    auto atom_reply = unique_C_ptr<xcb_intern_atom_reply_t> {
        xcb_intern_atom_reply(connection, atom_cookie, nullptr)
    };

    auto property_cookie = xcb_get_property_unchecked(
            connection, 0, window, atom_reply->atom, XCB_ATOM_ANY, 0, 1);
    auto property_reply = unique_C_ptr<xcb_get_property_reply_t> {
        xcb_get_property_reply(connection, property_cookie, nullptr),
    };

    auto *property_value = xcb_get_property_value(property_reply.get());
    auto property_length = xcb_get_property_value_length(property_reply.get());
    if (property_length != sizeof(int)) {
        return -1;
    }

    return *reinterpret_cast<int*>(property_value);
}

auto X11Util::get_pid_window_map() const -> std::unordered_map<int, xcb_window_t>
{
    std::unordered_map<int, xcb_window_t> res;
    for (auto window: get_server_window_ids()) {
        auto pid = get_window_pid(window);
        res.insert_or_assign(pid, window);
    }
    return res;
}

auto X11Util::window_has_property(xcb_window_t window, xcb_atom_t property, xcb_atom_t type) const -> bool
{
    auto cookie = xcb_get_property_unchecked(connection, 0, window, property, type, 0, 4);
    auto reply = unique_C_ptr<xcb_get_property_reply_t> {
        xcb_get_property_reply(connection, cookie, nullptr)
    };
    if (reply.get() == nullptr) {
        return false;
    }
    return xcb_get_property_value_length(reply.get()) != 0;
}

auto X11Util::get_window_dimensions(xcb_window_t window) const -> std::pair<int, int>
{
    auto cookie = xcb_get_geometry_unchecked(connection, window);
    auto reply = unique_C_ptr<xcb_get_geometry_reply_t> {
        xcb_get_geometry_reply(connection, cookie, nullptr)
    };
    if (reply.get() == nullptr) {
        return std::make_pair(0, 0);
    }
    return std::make_pair(reply->width, reply->height);
}

auto X11Util::get_parent_window(int pid) const -> xcb_window_t
{
    auto wid = os::getenv("WINDOWID");
    if (pid == os::get_pid() && wid.has_value()) {
        return std::stoi(wid.value());
    }

    auto pid_window_map = get_pid_window_map();
    auto ppids = util::get_process_tree(pid);
    for (const auto& ppid: ppids) {
        auto search = pid_window_map.find(ppid);
        if (search != pid_window_map.end()) {
            return search->second;
        }
    }
    return -1;
}

auto X11Util::is_connected() const -> bool
{
    return connected;
}
