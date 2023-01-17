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

#ifndef __TERMINAL__
#define __TERMINAL__

#include "process_info.hpp"
#include "window.hpp"

#include <xcb/xproto.h>
#include <memory>
#include <utility>

class Terminal
{
public:
    Terminal(int const& pid,
            xcb_window_t const& parent,
            xcb_connection_t *connection,
            xcb_screen_t *screen);
    ~Terminal();

    auto create_window(int x, int y, int max_width, int max_height) -> void;
    auto destroy_window() -> void;
    auto get_window_id() -> xcb_window_t;
    auto get_window_dimensions() -> std::pair<int, int>;

private:
    ProcessInfo proc;

    auto get_terminal_size() -> void;
    auto guess_padding(short chars, short pixels) -> double;
    auto guess_font_size(short chars, short pixels, double padding) -> double;

    xcb_window_t parent;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    int pty_fd;
    short rows;
    short cols;
    short xpixel;
    short ypixel;

    double padding_horizontal;
    double padding_vertical;
    double font_width;
    double font_height;

    std::unique_ptr<Window> window;
};

#endif
