#include "terminal.hpp"

#include <iostream>
#include <xcb/xcb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>

Terminal::Terminal(int const& pid,
        xcb_window_t const& parent,
        xcb_connection_t *connection,
        xcb_screen_t *screen):
parent(parent),
proc(ProcessInfo(pid)),
connection(connection),
screen(screen)
{
    this->pty_fd = open(proc.pty_path.c_str(), O_NONBLOCK);
    this->get_terminal_size();
}

Terminal::~Terminal()
{
    close(this->pty_fd);
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

auto Terminal::create_window(int x, int y, int max_width, int max_height) -> void
{
    double w_x = x * this->font_width,
           w_y = y * this->font_height,
           w_width = max_width * this->font_width,
           w_height = max_height * this->font_height;
    this->window = std::make_unique<Window>
        (this->connection, this->screen, this->parent);
    this->window->create(w_x, w_y, w_width, w_height);
}

auto Terminal::destroy_window() -> void
{
    this->window.reset();
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

auto Terminal::get_window_id() -> xcb_window_t
{
    if (!this->window.get()) return 0;
    return this->window->get_id();
}

