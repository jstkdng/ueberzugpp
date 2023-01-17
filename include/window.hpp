#ifndef __WINDOW__
#define __WINDOW__

#include <xcb/xproto.h>
#include <thread>

class Window
{
public:
    Window(xcb_connection_t *connection, xcb_screen_t *screen, xcb_window_t parent);
    ~Window();

    auto create(int x, int y, int max_width, int max_height) -> void;
    auto get_id() -> xcb_window_t;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t parent;
    xcb_window_t window;
};

#endif
