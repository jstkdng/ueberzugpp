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

    auto create_window() -> void;
    auto destroy_window() -> void;
    auto get_window_id() -> xcb_window_t;

private:
    ProcessInfo proc;

    xcb_window_t parent;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    std::unique_ptr<Window> window;
};

#endif
