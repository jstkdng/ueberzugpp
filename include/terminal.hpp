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

#include <string>
#include <string_view>

#include <termios.h>
#include <spdlog/spdlog.h>

class X11Util;

class Terminal
{
public:
    Terminal();
    ~Terminal();

    uint16_t font_width;
    uint16_t font_height;
    uint16_t padding_horizontal;
    uint16_t padding_vertical;
    uint16_t rows;
    uint16_t cols;
    std::string term;
    std::string term_program;
    std::string detected_output;

private:
    auto get_terminal_size() -> void;
    static auto guess_padding(uint16_t chars, double pixels) -> double;
    static auto guess_font_size(uint16_t chars, double pixels, double padding) -> double;
    static auto read_raw_str(std::string_view esc) -> std::string;

    void init_termios();
    void reset_termios();

    void check_sixel_support();
    void check_kitty_support();
    void check_iterm2_support();
    void check_x11_support();

    void get_terminal_size_escape_code();
    void get_terminal_size_xtsm();
    void get_fallback_terminal_sizes();

    void open_first_pty();
    void set_detected_output();

    int pty_fd;
    int pid;
    int xpixel;
    int ypixel;
    uint16_t fallback_xpixel = 0;
    uint16_t fallback_ypixel = 0;

    bool supports_sixel = false;
    bool supports_kitty = false;
    bool supports_x11 = false;
    bool supports_iterm2 = false;

    std::shared_ptr<Flags> flags;
    std::shared_ptr<spdlog::logger> logger;
#ifdef ENABLE_X11
    std::unique_ptr<X11Util> xutil;
#endif

    struct termios old_term;
    struct termios new_term;
};

#endif
