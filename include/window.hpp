#ifndef __WINDOW__
#define __WINDOW__

#include <xcb/xproto.h>
#include <thread>
#include <utility>

class Window
{
public:
    Window(xcb_connection_t *connection,
            xcb_screen_t *screen,
            xcb_window_t parent,
            int x, int y, int max_width, int max_height);
    ~Window();

    auto get_id() -> xcb_window_t;
    auto get_dimensions() -> std::pair<int, int>;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t parent;
    xcb_window_t window;

    int width;
    int height;
};

#endif
