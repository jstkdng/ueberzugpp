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
    term = os::getenv("TERM").value_or("xterm-256color");
    term_program = os::getenv("TERM_PROGRAM").value_or("");
    open_first_pty();
    get_terminal_size();
}

Terminal::~Terminal()
{
    close(this->pty_fd);
}

void Terminal::reload()
{
    get_terminal_size();
}

auto Terminal::get_terminal_size() -> void
{
    struct winsize sz;
    ioctl(pty_fd, TIOCGWINSZ, &sz);
    cols = sz.ws_col;
    rows = sz.ws_row;
    xpixel = sz.ws_xpixel;
    ypixel = sz.ws_ypixel;

    if (flags.use_escape_codes) {
        init_termios();
        if (xpixel == 0 && ypixel == 0) get_terminal_size_escape_code();
        if (flags.output.empty()) {
            check_sixel_support();
            check_kitty_support();
        }
        reset_termios();
    }
    if (xpixel == 0 && ypixel == 0) get_terminal_size_x11();

    double padding_horiz = guess_padding(cols, xpixel);
    double padding_vert = guess_padding(rows, ypixel);

    padding_horizontal = std::max(padding_horiz, padding_vert);
    padding_vertical = padding_horizontal;
    font_width =
        guess_font_size(cols, xpixel, padding_horizontal);
    font_height =
        guess_font_size(rows, ypixel, padding_vertical);
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
    auto resp = read_raw_str("\e[14t").erase(0, 4);
    if (resp.empty()) return;
    auto sizes = util::str_split(resp, ";");
    ypixel = std::stoi(sizes[0]);
    xpixel = std::stoi(sizes[1]);
}

void Terminal::check_sixel_support()
{
    // some terminals support sixel but don't respond to escape sequences
    auto supported_terms = std::unordered_set<std::string_view> {
        "yaft-256color"
    };
    auto resp = read_raw_str("\e[?1;1;0S").erase(0, 3);
    auto vals = util::str_split(resp, ";");
    if (vals.size() > 2 || supported_terms.contains(term)) {
        flags.output = "sixel";
    }
}

void Terminal::check_kitty_support()
{
    auto resp = read_raw_str("\e_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\e\\\e[c");
    if (resp.find("OK") != std::string::npos && term_program != "WezTerm") {
        flags.output = "kitty";
    }
}

auto Terminal::read_raw_str(const std::string& esc) -> std::string
{
    char c;
    std::stringstream ss;
    std::cout << esc << std::flush;
    struct pollfd input[1] = {{.fd = STDIN_FILENO, .events = POLLIN}};
    while (true) {
        // some terminals take some time to write, 100ms seems like enough
        // time to wait for input
        int r = poll(input, 1, 100);
        if (r <= 0) return "";
        r = read(STDIN_FILENO, &c, 1);
        if (r == -1) return "";
        if (c == esc.back()) break;
        ss << c;
    }
    return ss.str();
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
    if (!xutil.connected) return;
    auto window = xutil.get_parent_window(os::get_pid());
    auto dims = xutil.get_window_dimensions(window);
    xpixel = dims.first;
    ypixel = dims.second;
}

void Terminal::open_first_pty()
{
    auto tree = util::get_process_tree(pid);
    tree.pop_back();
    std::reverse(tree.begin(), tree.end());
    for (const auto& pid: tree) {
        auto proc = Process(pid);
        pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
        if (pty_fd != -1) return;
    }
    pty_fd = STDOUT_FILENO;
}
