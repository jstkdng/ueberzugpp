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

#ifndef __X11_CANVAS__
#define __X11_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "window.hpp"

#include <xcb/xproto.h>
#include <memory>
#include <vector>
#include <unordered_map>

class X11Canvas : public Canvas
{
public:
    X11Canvas();
    ~X11Canvas();

    auto create(int x, int y, int max_width, int max_height) -> void override;
    auto draw(Image& image) -> void override;
    auto clear() -> void override;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    std::vector<std::unique_ptr<Window>> windows;

    // utility functions
    auto get_server_window_ids() -> std::vector<xcb_window_t>;
    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void;
    auto get_window_pid(xcb_window_t window) -> unsigned int;
    auto get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>;
};

#endif
