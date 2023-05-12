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
#include "flags.hpp"
#include "process.hpp"
#ifdef ENABLE_X11
#   include "util/x11.hpp"
#endif

#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>

Terminal::Terminal(int pid, Flags& flags):
pid(pid), flags(flags)
{
    logger = spdlog::get("terminal");
    term = os::getenv("TERM").value_or("xterm-256color");
    term_program = os::getenv("TERM_PROGRAM").value_or("");
#ifdef ENABLE_X11
    xutil = std::make_unique<X11Util>();
#endif
    open_first_pty();
    get_terminal_size();
}

Terminal::~Terminal()
{
    if (pty_fd > 0) {
        close(this->pty_fd);
    }
}

void Terminal::reload()
{
    get_terminal_size();
}

auto Terminal::get_terminal_size() -> void
{
    struct winsize size;
    ioctl(pty_fd, TIOCGWINSZ, &size);
    cols = size.ws_col;
    rows = size.ws_row;
    xpixel = size.ws_xpixel;
    ypixel = size.ws_ypixel;
    logger->debug("Initial sizes: COLS={} ROWS={} XPIXEL={} YPIXEL={}", cols, rows, xpixel, ypixel);

#ifdef ENABLE_X11
    if (xutil->is_connected()) {
        detected_output = "x11";
    }
    if (xpixel == 0 || ypixel == 0) {
        get_terminal_size_x11();
    }
#endif
    if (flags.use_escape_codes) {
        init_termios();
        if (xpixel == 0 || ypixel == 0) {
            get_terminal_size_escape_code();
        }
        check_sixel_support();
        check_kitty_support();
        reset_termios();
    }

    double padding_horiz = guess_padding(cols, xpixel);
    double padding_vert = guess_padding(rows, ypixel);

    padding_horizontal = std::max(padding_horiz, padding_vert);
    padding_vertical = padding_horizontal;
    font_width =
        guess_font_size(cols, xpixel, padding_horizontal);
    font_height =
        guess_font_size(rows, ypixel, padding_vertical);

    logger->debug("font_width={} font_height={}", font_width, font_height);
}

auto Terminal::guess_padding(int chars, double pixels) -> double
{
    double font_size = std::floor(pixels / chars);
    return (pixels - font_size * chars) / 2;
}

auto Terminal::guess_font_size(int chars, double pixels, double padding)
    -> double
{
    return (pixels - 2 * padding) / chars;
}

void Terminal::get_terminal_size_escape_code()
{
    logger->debug("Obtaining sizes from escape codes");
    auto resp = read_raw_str("\e[14t").erase(0, 4);
    if (resp.empty()) {
        return;
    }
    auto sizes = util::str_split(resp, ";");
    ypixel = std::stoi(sizes[0]);
    xpixel = std::stoi(sizes[1]);
    logger->debug("New sizes: XPIXEL={} YPIXEL={}", xpixel, ypixel);
}

void Terminal::check_sixel_support()
{
    // some terminals support sixel but don't respond to escape sequences
    auto supported_terms = std::unordered_set<std::string_view> {
        "yaft-256color"
    };
    auto resp = read_raw_str("\e[?1;1;0S").erase(0, 3);
    auto vals = util::str_split(resp, ";");
    if ((vals.size() > 2 || supported_terms.contains(term)) && detected_output.empty()) {
        detected_output = "sixel";
    }
}

void Terminal::check_kitty_support()
{
    auto resp = read_raw_str("\e_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\e\\\e[c");
    if (resp.find("OK") != std::string::npos) {
        detected_output = "kitty";
    }
}

auto Terminal::read_raw_str(const std::string& esc) -> std::string
{
    char chr = 0;
    std::stringstream sstream;
    std::cout << esc << std::flush;
    //struct pollfd input[1] = {{.fd = STDIN_FILENO, .events = POLLIN}};

    std::array<struct pollfd, 1> input;
    input.fill({.fd = STDIN_FILENO, .events = POLLIN});

    const int waitms = 100;
    while (true) {
        // some terminals take some time to write, 100ms seems like enough
        // time to wait for input
        size_t res = poll(input.data(), 1, waitms);
        if (res <= 0) {
            return "";
        }
        res = read(STDIN_FILENO, &chr, 1);
        if (res == -1) {
            return "";
        }
        if (chr == esc.back()) {
            break;
        }
        sstream << chr;
    }
    return sstream.str();
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

void Terminal::get_terminal_size_x11()
{
#ifdef ENABLE_X11
    logger->debug("Obtaining sizes from x11 server");
    if (!xutil->is_connected()) {
        return;
    }
    auto window = xutil->get_parent_window(os::get_pid());
    auto dims = xutil->get_window_dimensions(window);
    xpixel = dims.first;
    ypixel = dims.second;
    logger->debug("X11 sizes: XPIXEL={} YPIXEL={}", xpixel, ypixel);
#endif
}

void Terminal::open_first_pty()
{
    auto tree = util::get_process_tree(pid);
    tree.pop_back();
    std::reverse(tree.begin(), tree.end());
    for (const auto& pid: tree) {
        auto proc = Process(pid);
        pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
        if (pty_fd != -1) {
            return;
        }
    }
    pty_fd = STDOUT_FILENO;
}
