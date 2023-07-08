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
#include "flags.hpp"

#include <xcb/xcb.h>
#include <xcb/res.h>

#include <algorithm>
#include <utility>
#include <string>
#include <stack>

X11Util::X11Util():
connection(xcb_connect(nullptr, nullptr))
{
    const auto flags = Flags::instance();
    const auto xdg_session = os::getenv("XDG_SESSION_TYPE").value_or("");
    if (xcb_connection_has_error(connection) == 0) {
        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
        if (xdg_session != "wayland" || flags->output == "x11") {
            connected = true;
        }
    }
}

X11Util::X11Util(xcb_connection_t* connection):
connection(connection),
screen(xcb_setup_roots_iterator(xcb_get_setup(connection)).data),
owns_connection(false)
{}

X11Util::~X11Util()
{
    if (owns_connection) {
        xcb_disconnect(connection);
    }
}

auto X11Util::get_server_window_ids() const -> std::vector<xcb_window_t>
{
    const int num_clients = 256;
    std::vector<xcb_window_t> windows;
    std::stack<xcb_query_tree_cookie_t> cookies_st;
    windows.reserve(num_clients);

    cookies_st.push(xcb_query_tree_unchecked(connection, screen->root));

    while (!cookies_st.empty()) {
        const auto cookie = cookies_st.top();
        cookies_st.pop();

        const auto reply = unique_C_ptr<xcb_query_tree_reply_t> {
            xcb_query_tree_reply(connection, cookie, nullptr)
        };
        if (!reply) {
            continue;
        }

        const auto num_children = xcb_query_tree_children_length(reply.get());
        if (num_children == 0) {
            continue;
        }
        const auto *children = xcb_query_tree_children(reply.get());
        for (int i = 0; i < num_children; ++i) {
            const auto child = children[i];
            const bool is_complete_window = window_has_properties(child, {XCB_ATOM_WM_CLASS, XCB_ATOM_WM_NAME});
            if (is_complete_window) {
                windows.push_back(child);
            }
            cookies_st.push(xcb_query_tree_unchecked(connection, child));
        }
    }
    return windows;
}

auto X11Util::get_pid_window_map() const -> std::unordered_map<uint32_t, xcb_window_t>
{
    const auto windows = get_server_window_ids();
    std::unordered_map<uint32_t, xcb_window_t> res;
    std::vector<xcb_res_query_client_ids_cookie_t> cookies;
    res.reserve(windows.size());
    cookies.reserve(windows.size());

    struct xcb_res_client_id_spec_t spec;
    spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

    // bulk request pids
    for (const auto window: windows) {
        spec.client = window;
        cookies.push_back(xcb_res_query_client_ids_unchecked(connection, 1, &spec));
    }

    // process replies
    auto win_iter = windows.cbegin();
    for (const auto cookie: cookies) {
        const auto reply = unique_C_ptr<xcb_res_query_client_ids_reply_t> {
            xcb_res_query_client_ids_reply(connection, cookie, nullptr)
        };
        if (!reply) {
            continue;
        }
        const auto iter = xcb_res_query_client_ids_ids_iterator(reply.get());
        const auto pid = *xcb_res_client_id_value_value(iter.data);
        res.insert_or_assign(pid, *win_iter);
        std::advance(win_iter, 1);
    }
    return res;
}

auto X11Util::window_has_properties(xcb_window_t window, std::initializer_list<xcb_atom_t> properties) const -> bool
{
    std::vector<xcb_get_property_cookie_t> cookies;
    cookies.reserve(properties.size());
    for (const auto prop: properties) {
        cookies.push_back(xcb_get_property_unchecked(connection, 0, window, prop, XCB_ATOM_ANY, 0, 4));
    }
    return std::ranges::all_of(std::as_const(cookies), [&] (xcb_get_property_cookie_t cookie) -> bool {
        const auto reply = unique_C_ptr<xcb_get_property_reply_t> {
            xcb_get_property_reply(connection, cookie, nullptr)
        };
        return reply && xcb_get_property_value_length(reply.get()) != 0;
    });
}

auto X11Util::get_window_dimensions(xcb_window_t window) const -> std::pair<int, int>
{
    const auto cookie = xcb_get_geometry_unchecked(connection, window);
    const auto reply = unique_C_ptr<xcb_get_geometry_reply_t> {
        xcb_get_geometry_reply(connection, cookie, nullptr)
    };
    if (!reply) {
        return std::make_pair(0, 0);
    }
    return std::make_pair(reply->width, reply->height);
}

auto X11Util::get_parent_window(int pid) const -> xcb_window_t
{
    const auto wid = os::getenv("WINDOWID");
    if (wid.has_value()) {
        return std::stoi(wid.value());
    }

    const auto pid_window_map = get_pid_window_map();
    const auto tree = util::get_process_tree(pid);
    for (const auto spid: tree) {
        const auto win = pid_window_map.find(spid);
        if (win != pid_window_map.end()) {
            return win->second;
        }
    }

    return 0;
}
