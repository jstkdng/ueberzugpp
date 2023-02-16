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
#include <stdexcept>
#include <unordered_set>

Terminal::Terminal(ProcessInfo pid):
proc(pid)
{
    pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
    if (pty_fd == -1) {
        throw std::runtime_error("unable to open pty.");
    }
    name = os::getenv("TERM").value_or("xterm-256color");
    get_terminal_size();
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
    ioctl(pty_fd, TIOCGWINSZ, &sz);
    cols = sz.ws_col;
    rows = sz.ws_row;
    xpixel = sz.ws_xpixel;
    ypixel = sz.ws_ypixel;
    if (cols <= 0 || rows <= 0 || xpixel <= 0 || ypixel <= 0) {
        throw std::runtime_error("received wrong terminal sizes.");
    }

    double padding_horiz = guess_padding(cols, xpixel);
    double padding_vert = guess_padding(rows, ypixel);

    padding_horizontal = std::max(padding_horiz, padding_vert);
    padding_vertical = padding_horiz;
    font_width =
        guess_font_size(cols, xpixel, padding_horizontal);
    font_height =
        guess_font_size(rows, ypixel, padding_vertical);
}

auto Terminal::guess_padding(short chars, short pixels)
    -> double
{
    double font_size = std::floor(static_cast<double>(pixels) / chars);
    return (- font_size * chars + pixels) / 2;
}

auto Terminal::guess_font_size(short chars, short pixels, double padding)
    -> double
{
    return (pixels - 2 * padding) / chars;
}
