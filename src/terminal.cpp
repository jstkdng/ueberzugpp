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

#include "terminal.hpp"
#include "os.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>
#include <iostream>
#include <unordered_set>

Terminal::Terminal(ProcessInfo pid):
proc(pid)
{
    this->pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
    this->name = os::getenv("TERM").value_or("xterm-256color");
    this->get_terminal_size();
}

Terminal::~Terminal()
{
    close(this->pty_fd);
}

auto Terminal::supports_sixel() const -> bool
{
    std::unordered_set<std::string> supported_terms {
        "contour", "foot", "xterm-256color", "yaft-256color"
    };
    return supported_terms.contains(name);
}

auto Terminal::get_terminal_size() -> void
{
    struct winsize sz;
    ioctl(this->pty_fd, TIOCGWINSZ, &sz);
    this->cols = sz.ws_col;
    this->rows = sz.ws_row;
    this->xpixel = sz.ws_xpixel;
    this->ypixel = sz.ws_ypixel;

    double padding_horiz = this->guess_padding(this->cols, this->xpixel);
    double padding_vert = this->guess_padding(this->rows, this->ypixel);

    this->padding_horizontal = std::max(padding_horiz, padding_vert);
    this->padding_vertical = padding_horiz;
    this->font_width =
        this->guess_font_size(this->cols, this->xpixel, this->padding_horizontal);
    this->font_height =
        this->guess_font_size(this->rows, this->ypixel, this->padding_vertical);
}

auto Terminal::guess_padding(short chars, short pixels)
    -> double
{
    double font_size = floor(static_cast<double>(pixels) / chars);
    return (- font_size * chars + pixels) / 2;
}

auto Terminal::guess_font_size(short chars, short pixels, double padding)
    -> double
{
    return (pixels - 2 * padding) / chars;
}
