#ifndef __TERMINAL__
#define __TERMINAL__

#include "process_info.hpp"
#include "window.hpp"

#include <xcb/xproto.h>
#include <memory>

class Terminal
{
public:
    Terminal(int const& pid,
            xcb_window_t const& parent,
            xcb_connection_t *connection,
            xcb_screen_t *screen);
    ~Terminal();

    auto create_window(int x, int y, int max_height, int max_width) -> void;
    auto destroy_window() -> void;
    auto get_window_id() -> xcb_window_t;

private:
    ProcessInfo proc;

    auto get_terminal_size() -> void;
    auto guess_padding(short chars, short pixels) -> double;
    auto guess_font_size(short chars, short pixels, double padding) -> double;

    xcb_window_t parent;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    int pty_fd;
    short rows;
    short cols;
    short xpixel;
    short ypixel;

    double padding_horizontal;
    double padding_vertical;
    double font_width;
    double font_height;

    std::unique_ptr<Window> window;
};

#endif
