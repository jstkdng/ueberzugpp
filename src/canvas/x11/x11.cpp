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

#include "x11.hpp"
#include "util.hpp"
#include "os.hpp"
#include "tmux.hpp"

#include <xcb/xcb.h>

struct free_delete
{
    void operator()(void* x) { free(x); }
};

X11Canvas::X11Canvas()
{
    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
}

X11Canvas::~X11Canvas()
{
    windows.clear();
    xcb_disconnect(connection);
}

auto X11Canvas::draw(const Image& image) -> void
{
    for (const auto& window: windows) {
        window->draw(image);
    }
}

auto X11Canvas::create(int x, int y, int max_width, int max_height) -> void
{
    std::vector<ProcessInfo> client_pids {ProcessInfo(os::get_pid())};
    std::unordered_map<unsigned int, xcb_window_t> pid_window_map;

    auto wid = os::getenv("WINDOWID");

    if (tmux::is_used()) {
        client_pids = tmux::get_client_pids().value();
        pid_window_map = this->get_pid_window_map();
    } else if (wid.has_value()) {
        // if WID exists prevent doing any calculations
        auto proc = client_pids.front();
        windows.push_back(std::make_unique<Window>(connection, screen,
                    std::stoi(wid.value()), x, y, max_width, max_height));
        return;
    }

    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_parent_pids(pid);
        for (const auto& ppid: ppids) {
            if (!pid_window_map.contains(ppid.pid)) continue;
            windows.push_back(std::make_unique<Window>(connection, screen,
                    pid_window_map[ppid.pid], x, y, max_width, max_height));
        }
    }
}

auto X11Canvas::clear() -> void
{
    windows.clear();
}

auto X11Canvas::get_server_window_ids() -> std::vector<xcb_window_t>
{
    auto cookie = xcb_query_tree_unchecked(this->connection, this->screen->root);
    std::vector<xcb_window_t> windows;
    get_server_window_ids_helper(windows, cookie);
    return windows;
}

auto X11Canvas::get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void
{
    std::unique_ptr<xcb_query_tree_reply_t, free_delete> reply {
        xcb_query_tree_reply(this->connection, cookie, nullptr)
    };
    int num_children = xcb_query_tree_children_length(reply.get());

    if (!num_children) return;

    auto children = xcb_query_tree_children(reply.get());
    std::vector<xcb_query_tree_cookie_t> cookies;

    for (int i = 0; i < num_children; ++i) {
        auto child = children[i];
        bool is_complete_window = (
            util::window_has_property(this->connection, child, XCB_ATOM_WM_NAME, XCB_ATOM_STRING) &&
            util::window_has_property(this->connection, child, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING) &&
            util::window_has_property(this->connection, child, XCB_ATOM_WM_NORMAL_HINTS)
        );
        if (is_complete_window) windows.push_back(child);
        cookies.push_back(xcb_query_tree_unchecked(this->connection, child));
    }

    for (auto new_cookie: cookies) {
        this->get_server_window_ids_helper(windows, new_cookie);
    }
}

auto X11Canvas::get_window_pid(xcb_window_t window) -> unsigned int
{
    std::string atom_str = "_NET_WM_PID";

    auto atom_cookie = xcb_intern_atom_unchecked
        (this->connection, false, atom_str.size(), atom_str.c_str());
    auto atom_reply = std::unique_ptr<xcb_intern_atom_reply_t, free_delete> {
        xcb_intern_atom_reply(this->connection, atom_cookie, nullptr)
    };

    auto property_cookie = xcb_get_property_unchecked(
            this->connection, false, window, atom_reply->atom,
            XCB_ATOM_ANY, 0, 1);
    auto property_reply = std::unique_ptr<xcb_get_property_reply_t, free_delete> {
        xcb_get_property_reply(this->connection, property_cookie, nullptr),
    };

    return *reinterpret_cast<unsigned int*>
        (xcb_get_property_value(property_reply.get()));
}

auto X11Canvas::get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>
{
    std::unordered_map<unsigned int, xcb_window_t> res;
    for (auto window: this->get_server_window_ids()) {
        auto pid = this->get_window_pid(window);
        res[pid] = window;
    }
    return res;
}

