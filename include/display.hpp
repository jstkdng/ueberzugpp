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

#ifndef __DISPLAY__
#define __DISPLAY__

#include "logging.hpp"
#include "image.hpp"

#include <memory>
#include <string>
#include <xcb/xproto.h>
#include <thread>
#include <vector>
#include <unordered_map>

class Display
{
public:
    Display(Logging &logger);
    ~Display();

    void create_window();
    void load_image(std::string filename);
    std::thread spawn_event_handler();
    void handle_events();
    void destroy_image();
    auto get_server_window_ids() -> std::vector<xcb_window_t>;
    auto get_window_pid(xcb_window_t window) -> int;
    auto get_parent_terminals() -> void;
    auto terminate_event_handler() -> void;

private:
    void draw_image();
    void trigger_redraw();
    auto send_expose_event(int x, int y) -> void;
    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void;
    auto get_pid_window_map() -> std::unordered_map<int, xcb_window_t>;

    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t window = 0;

    Logging &logger;
    std::unique_ptr<Image> image;
};


#endif
