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
#include "util.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <sstream>
#include <iostream>

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
        "contour", "foot", "xterm-256color-sixel", "yaft-256color",
        "BlackBox", "WezTerm"
    };
    auto term_program = os::getenv("TERM_PROGRAM");
    if (term_program.has_value()) {
        return supported_terms.contains(term_program.value());
    }
    return supported_terms.contains(name) && !os::getenv("VTE_VERSION").has_value();
}

auto Terminal::get_terminal_size() -> void
{
    struct winsize sz;
    ioctl(pty_fd, TIOCGWINSZ, &sz);
    cols = sz.ws_col;
    rows = sz.ws_row;
    xpixel = sz.ws_xpixel;
    ypixel = sz.ws_ypixel;
    if (cols <= 0 || rows <= 0) {
        throw std::runtime_error("received wrong terminal sizes.");
    }
    if (xpixel <= 0 || ypixel <= 0) {
        get_terminal_size_escape_code();
        if (xpixel <= 0 || ypixel <= 0) {
            throw std::runtime_error("received wrong terminal sizes.");
        }
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

auto Terminal::get_terminal_size_escape_code() -> void
{
    init_termios();
    char c;
    std::stringstream ss;
    std::cout << "\033[14t" << std::flush;
    while (true) {
        read(0, &c, 1);
        if (c == 't') break;
        ss << c;
    }
    reset_termios();
    std::stringstream newss (ss.str().erase(0, 4));
    std::string segment;
    std::vector<std::string> seglist;
    while (std::getline(newss, segment, ';')) {
        seglist.push_back(segment);
    }
    ypixel = std::stoi(seglist.at(0));
    xpixel = std::stoi(seglist.at(1));
}

auto Terminal::init_termios() -> void
{
    tcgetattr(0, &old_term); /* grab old terminal i/o settings */
    new_term = old_term; /* make new settings same as old settings */
    new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
    new_term.c_lflag &= ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &new_term); /* use these new terminal i/o settings now */
}

auto Terminal::reset_termios() -> void
{
    tcsetattr(0, TCSANOW, &old_term);
}
