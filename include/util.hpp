#ifndef __NAMESPACE_UTIL__
#define __NAMESPACE_UTIL__

#include <string>
#include <vector>
#include <xcb/xproto.h>

namespace util
{
    auto str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>;
    auto get_parent_pids(int const& pid) -> std::vector<int>;
    auto window_has_property(
            xcb_connection_t *connection,
            xcb_window_t window,
            xcb_atom_t property,
            xcb_atom_t type = XCB_ATOM_ANY) -> bool;
}

#endif
