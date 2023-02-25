#ifndef __X11_UTIL__
#define __X11_UTIL__

#include <xcb/xproto.h>
#include <vector>
#include <unordered_map>

class X11Util
{
public:
    bool connected = false;

    X11Util();
    ~X11Util();

    auto get_server_window_ids() const -> std::vector<xcb_window_t>;
    auto get_pid_window_map() const -> std::unordered_map<unsigned int, xcb_window_t>;
    auto get_window_dimensions(xcb_window_t window) const -> std::pair<int, int>;
    auto get_parent_window(int pid) const -> xcb_window_t;

    bool window_has_property(xcb_window_t window, xcb_atom_t property, xcb_atom_t type = XCB_ATOM_ANY) const;
    int get_window_pid(xcb_window_t window) const;

private:
    xcb_connection_t* connection;
    xcb_screen_t* screen;

    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows,
        xcb_query_tree_cookie_t cookie) const -> void;
};

#endif
