#ifndef __X11_CANVAS__
#define __X11_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "terminal.hpp"
#include "window.hpp"

#include <xcb/xproto.h>
#include <xcb/xcb_image.h>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>
#include <unordered_map>

class X11Canvas : public Canvas
{
public:
    X11Canvas(const Terminal& terminal);
    ~X11Canvas();

    auto create(int x, int y, int max_width, int max_height) -> void override;
    auto draw(const Image& image) -> void override;
    auto clear() -> void override;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    std::unordered_set<std::unique_ptr<Window>> windows;

    // utility functions
    auto get_server_window_ids() -> std::vector<xcb_window_t>;
    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void;
    auto get_window_pid(xcb_window_t window) -> unsigned int;
    auto get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>;
};

#endif
