#include "util.hpp"
#include "process_info.hpp"
#include "free_delete.hpp"

#include <xcb/xcb.h>
#include <memory>
#include <xcb/xproto.h>

auto util::str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>
{
    auto start = 0U;
    auto end = str.find(delim);

    std::vector<std::string> res;

    while (end != std::string::npos) {
        res.push_back(str.substr(start, end - start));
        start = end + delim.length();
        end = str.find(delim, start);
    }

    return res;
}

auto util::get_parent_pids(int const& pid) -> std::vector<int>
{
    std::vector<int> res;
    ProcessInfo proc(pid);
    while (proc.ppid != 1) {
        res.push_back(proc.ppid);
        proc = ProcessInfo(proc.ppid);
    }
    return res;
}

auto util::window_has_property(
        xcb_connection_t *connection,
        xcb_window_t window,
        xcb_atom_t property,
        xcb_atom_t type) -> bool
{
    auto cookie = xcb_get_property(connection, false, window, property, type, 0, 4);
    auto reply = std::unique_ptr<xcb_get_property_reply_t, free_delete> {
        xcb_get_property_reply(connection, cookie, nullptr)
    };
    if (!reply.get()) return false;
    return xcb_get_property_value_length(reply.get()) != 0;
}

