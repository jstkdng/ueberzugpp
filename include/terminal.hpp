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

#include "flags.hpp"
#include "util/x11.hpp"

#include <string>
#include <termios.h>

class Terminal
{
public:
    Terminal(int pid, Flags& flags);
    ~Terminal();

    double font_width;
    double font_height;
    int rows;
    int cols;
    std::string term;
    std::string term_program;

    void reload();

private:
    auto get_terminal_size() -> void;
    auto guess_padding(int chars, double pixels) -> double;
    auto guess_font_size(int chars, double pixels, double padding) -> double;

    void init_termios();
    void reset_termios();
    void check_sixel_support();
    void check_kitty_support();
    void get_terminal_size_escape_code();
    void get_terminal_size_x11();
    void open_first_pty();

    auto read_raw_str(const std::string& esc) -> std::string;

    int pty_fd;
    int pid;
    double padding_horizontal;
    double padding_vertical;
    int xpixel;
    int ypixel;

    Flags& flags;
    const X11Util xutil;

    struct termios old_term;
    struct termios new_term;
};

#endif
