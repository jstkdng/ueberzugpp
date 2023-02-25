#include "util/x11.hpp"
#include "util.hpp"
#include "os.hpp"

#include <xcb/xcb.h>
#include <string>
#include <memory>
#include <iostream>

struct free_delete
{
    void operator()(void* x) { free(x); }
};

X11Util::X11Util()
{
    connection = xcb_connect(nullptr, nullptr);
    if (!xcb_connection_has_error(connection)) {
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

auto X11Util::get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) const -> void
{
    auto reply = std::unique_ptr<xcb_query_tree_reply_t, free_delete> {
        xcb_query_tree_reply(connection, cookie, nullptr)
    };
    if (!reply.get()) throw std::runtime_error("UNABLE TO QUERY WINDOW TREE");
    int num_children = xcb_query_tree_children_length(reply.get());

    if (!num_children) return;

    auto children = xcb_query_tree_children(reply.get());
    std::vector<xcb_query_tree_cookie_t> cookies;

    for (int i = 0; i < num_children; ++i) {
        auto child = children[i];
        bool is_complete_window = (
            window_has_property(child, XCB_ATOM_WM_NAME, XCB_ATOM_STRING) &&
            window_has_property(child, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING) &&
            window_has_property(child, XCB_ATOM_WM_NORMAL_HINTS)
        );
        if (is_complete_window) windows.push_back(child);
        cookies.push_back(xcb_query_tree_unchecked(connection, child));
    }

    for (auto new_cookie: cookies) {
        get_server_window_ids_helper(windows, new_cookie);
    }
}

int X11Util::get_window_pid(xcb_window_t window) const
{
    std::string atom_str = "_NET_WM_PID";

    auto atom_cookie = xcb_intern_atom_unchecked
        (connection, false, atom_str.size(), atom_str.c_str());
    auto atom_reply = std::unique_ptr<xcb_intern_atom_reply_t, free_delete> {
        xcb_intern_atom_reply(connection, atom_cookie, nullptr)
    };

    auto property_cookie = xcb_get_property_unchecked(
            connection, false, window, atom_reply->atom, XCB_ATOM_ANY, 0, 1);
    auto property_reply = std::unique_ptr<xcb_get_property_reply_t, free_delete> {
        xcb_get_property_reply(connection, property_cookie, nullptr),
    };

    return *reinterpret_cast<int*>
        (xcb_get_property_value(property_reply.get()));
}

auto X11Util::get_pid_window_map() const -> std::unordered_map<unsigned int, xcb_window_t>
{
    std::unordered_map<unsigned int, xcb_window_t> res;
    for (auto window: get_server_window_ids()) {
        auto pid = get_window_pid(window);
        res[pid] = window;
    }
    return res;
}

bool X11Util::window_has_property(xcb_window_t window, xcb_atom_t property, xcb_atom_t type) const
{
    auto cookie = xcb_get_property_unchecked(connection, false, window, property, type, 0, 4);
    auto reply = std::unique_ptr<xcb_get_property_reply_t, free_delete> {
        xcb_get_property_reply(connection, cookie, nullptr)
    };
    if (!reply.get()) return false;
    return xcb_get_property_value_length(reply.get()) != 0;
}

auto X11Util::get_window_dimensions(xcb_window_t window) const -> std::pair<int, int>
{
    auto cookie = xcb_get_geometry_unchecked(connection, window);
    auto reply = std::unique_ptr<xcb_get_geometry_reply_t, free_delete> {
        xcb_get_geometry_reply(connection, cookie, nullptr)
    };
    if (!reply.get()) return std::make_pair(0, 0);
    return std::make_pair(reply->width, reply->height);
}

auto X11Util::get_parent_window(int pid) const -> xcb_window_t
{
    auto wid = os::getenv("WINDOWID");
    if (pid == os::get_pid() && wid.has_value()) return std::stoi(wid.value());

    auto pid_window_map = get_pid_window_map();
    auto ppids = util::get_process_tree(pid);
    for (const auto& ppid: ppids) {
        auto search = pid_window_map.find(ppid);
        if (search != pid_window_map.end()) return search->second;
    }
    return -1;
}
