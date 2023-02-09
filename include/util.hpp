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

#ifndef __NAMESPACE_UTIL__
#define __NAMESPACE_UTIL__

#include <string>
#include <vector>
#include <xcb/xproto.h>
#include "process_info.hpp"

namespace util
{
    auto str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>;
    auto get_parent_pids(const ProcessInfo& proc) -> std::vector<ProcessInfo>;
    auto window_has_property(
            xcb_connection_t *connection,
            xcb_window_t window,
            xcb_atom_t property,
            xcb_atom_t type = XCB_ATOM_ANY) -> bool;
}

#endif
