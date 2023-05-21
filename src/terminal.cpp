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
#include "tmux.hpp"
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
#include <gsl/util>

Terminal::Terminal():
pid(os::get_pid())
{
    flags = Flags::instance();
    logger = spdlog::get("terminal");
    term = os::getenv("TERM").value_or("xterm-256color");
    term_program = os::getenv("TERM_PROGRAM").value_or("");
#ifdef ENABLE_X11
    xutil = std::make_unique<X11Util>();
#endif
    logger->info(R"(TERM="{}", TERM_PROGRAM="{}")", term, term_program);
    open_first_pty();
    get_terminal_size();
    set_detected_output();
}

Terminal::~Terminal()
{
    if (pty_fd > 0) {
        close(this->pty_fd);
    }
}

auto Terminal::get_terminal_size() -> void
{
    struct winsize size;
    ioctl(pty_fd, TIOCGWINSZ, &size);
    cols = size.ws_col;
    rows = size.ws_row;
    xpixel = size.ws_xpixel;
    ypixel = size.ws_ypixel;
    logger->debug("ioctl sizes: COLS={} ROWS={} XPIXEL={} YPIXEL={}", cols, rows, xpixel, ypixel);

    get_fallback_terminal_sizes();

    check_x11_support();
    check_iterm2_support();
    if (flags->use_escape_codes) {
        init_termios();
        if (xpixel == 0 || ypixel == 0) {
            get_terminal_size_escape_code();
        }
        if (xpixel == 0 || ypixel == 0) {
            get_terminal_size_xtsm();
        }
        check_sixel_support();
        check_kitty_support();
        reset_termios();
    }

    if (xpixel == 0 || ypixel == 0) {
        xpixel = fallback_xpixel;
        ypixel = fallback_ypixel;
    }
    if (xpixel == 0 || ypixel == 0) {
        throw std::runtime_error("UNABLE TO CALCULATE TERMINAL SIZES");
    }

    const double padding_horiz = guess_padding(cols, xpixel);
    const double padding_vert = guess_padding(rows, ypixel);

    padding_horizontal = gsl::narrow_cast<uint16_t>(std::max(padding_horiz, padding_vert));
    padding_vertical = padding_horizontal;
    font_width = std::ceil(guess_font_size(cols, xpixel, padding_horizontal));
    font_height = std::ceil(guess_font_size(rows, ypixel, padding_vertical));

    if (xpixel < fallback_xpixel && ypixel < fallback_ypixel) {
        padding_horizontal = (fallback_xpixel - xpixel) / 2;
        padding_vertical = (fallback_ypixel - ypixel) / 2;
        font_width = gsl::narrow_cast<uint16_t>(xpixel / cols);
        font_height = gsl::narrow_cast<uint16_t>(ypixel / rows);
    }

    logger->debug("padding_horiz={} padding_vert={}", padding_horizontal, padding_vertical);
    logger->debug("font_width={} font_height={}", font_width, font_height);
}

void Terminal::set_detected_output()
{
    if (supports_sixel) {
        detected_output = "sixel";
    }
    if (supports_kitty) {
        detected_output = "kitty";
    }
    if (supports_iterm2) {
        detected_output = "iterm2";
    }
    if (supports_x11) {
        detected_output = "x11";
    }
    if (flags->output.empty()) {
        flags->output = detected_output;
    }
}

auto Terminal::guess_padding(uint16_t chars, double pixels) -> double
{
    const double font_size = std::floor(pixels / chars);
    return (pixels - font_size * chars) / 2;
}

auto Terminal::guess_font_size(uint16_t chars, double pixels, double padding) -> double
{
    return (pixels - 2 * padding) / chars;
}

void Terminal::get_terminal_size_escape_code()
{
    const auto resp = read_raw_str("\033[14t").erase(0, 4);
    if (resp.empty()) {
        return;
    }
    const auto sizes = util::str_split(resp, ";");
    ypixel = std::stoi(sizes[0]);
    xpixel = std::stoi(sizes[1]);
    // some old vte terminals respond to this values in a different order
    // assume everything older than 7000 is broken
    const auto vte_ver_str = os::getenv("VTE_VERSION").value_or("");
    if (!vte_ver_str.empty()) {
        const auto vte_ver = std::stoi(vte_ver_str);
        const auto working_ver = 7000;
        if (vte_ver <= working_ver) {
            std::swap(ypixel, xpixel);
        }
    }
    logger->debug("ESC sizes XPIXEL={} YPIXEL={}", xpixel, ypixel);
}

void Terminal::get_terminal_size_xtsm()
{
    const auto resp = read_raw_str("\033[?2;1;0S").erase(0, 3);
    if (resp.empty()) {
        return;
    }
    const auto sizes = util::str_split(resp, ";");
    if (sizes.size() != 4) {
        return;
    }
    ypixel = std::stoi(sizes[3]);
    xpixel = std::stoi(sizes[2]);
    logger->debug("XTSM sizes XPIXEL={} YPIXEL={}", xpixel, ypixel);
}

void Terminal::check_sixel_support()
{
    // some terminals support sixel but don't respond to escape sequences
    const auto supported_terms = std::unordered_set<std::string_view> {
        "yaft-256color", "iTerm.app"
    };
    const auto resp = read_raw_str("\033[?1;1;0S").erase(0, 3);
    const auto vals = util::str_split(resp, ";");
    if (vals.size() > 2 || supported_terms.contains(term) || supported_terms.contains(term_program)) {
        supports_sixel = true;
        logger->debug("sixel is supported");
    } else {
        logger->debug("sixel is not supported");
    }
}

void Terminal::check_kitty_support()
{
    const auto resp = read_raw_str("\033_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\033\\\033[c");
    if (resp.find("OK") != std::string::npos) {
        supports_kitty = true;
        logger->debug("kitty is supported");
    } else {
        logger->debug("kitty is not supported");
    }
}

void Terminal::check_iterm2_support()
{
    const auto supported_terms = std::unordered_set<std::string_view> {
        "WezTerm", "iTerm.app"
    };
    if (supported_terms.contains(term_program)) {
        supports_iterm2 = true;
        logger->debug("iterm2 is supported");
    } else {
        logger->debug("iterm2 is not supported");
    }
}

void Terminal::check_x11_support()
{
#ifdef ENABLE_X11
    if (xutil->connected) {
        supports_x11 = true;
        logger->debug("X11 is supported");
    } else {
        logger->debug("X11 is not supported");
    }
#else
    logger->debug("x11 is not supported");
#endif
}

auto Terminal::read_raw_str(const std::string_view esc) -> std::string
{
    char chr = 0;
    std::string str;
    const auto waitms = 100;
    struct pollfd input;
    input.fd = STDIN_FILENO;
    input.events = POLLIN;

    std::cout << esc << std::flush;
    while (true) {
        // some terminals take some time to write, 100ms seems like enough
        // time to wait for input
        const auto poll_res = poll(&input, 1, waitms);
        if (poll_res <= 0) {
            return "";
        }
        const auto read_res = read(STDIN_FILENO, &chr, 1);
        if (read_res == -1) {
            return "";
        }
        if (chr == esc.back()) {
            break;
        }
        str.push_back(chr);
    }
    return str;
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

void Terminal::get_fallback_terminal_sizes()
{
#ifdef ENABLE_X11
    if (!xutil->connected) {
        return;
    }
    const auto window = xutil->get_parent_window(os::get_pid());
    const auto dims = xutil->get_window_dimensions(window);
    fallback_xpixel = dims.first;
    fallback_ypixel = dims.second;
    logger->debug("X11 sizes: XPIXEL={} YPIXEL={}", fallback_xpixel, fallback_ypixel);
#endif
}

void Terminal::open_first_pty()
{
    std::vector<int> pids {pid};
    if (tmux::is_used()) {
        const auto clients = tmux::get_client_pids();
        if (clients.has_value()) {
            pids = clients.value();
        }
    }
    for (const auto& spid: pids) {
        const auto tree = util::get_process_tree(spid);
        for (const auto& tpid: tree) {
            const auto proc = Process(tpid);
            pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
            if (pty_fd != -1) {
                logger->debug("Opened {}" , proc.pty_path.c_str());
                return;
            }
        }
    }
    pty_fd = STDOUT_FILENO;
}
