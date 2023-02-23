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
#include "flags.hpp"

#include <string>
#include <termios.h>

class Terminal
{
public:
    Terminal(ProcessInfo pid, const Flags& flags);
    ~Terminal();

    ProcessInfo proc;
    double font_width;
    double font_height;
    std::string name;

    auto supports_sixel() const -> bool;

private:
    auto get_terminal_size() -> void;
    auto guess_padding(short chars, short pixels) -> double;
    auto guess_font_size(short chars, short pixels, double padding) -> double;

    auto init_termios() -> void;
    auto reset_termios() -> void;
    auto get_terminal_size_escape_code() -> void;

    int pty_fd;
    double padding_horizontal;
    double padding_vertical;
    short rows;
    short cols;
    short xpixel;
    short ypixel;

    const Flags& flags;

    struct termios old_term;
    struct termios new_term;
};

#endif
