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
#include "terminal.hpp"

#include <memory>
#include <string>
#include <thread>
#include <optional>
#include <xcb/xproto.h>
#include <vector>
#include <unordered_map>

class Display
{
public:
    Display(Logging &logger);
    ~Display();

    void load_image(std::string filename);
    void destroy_image();
    auto get_server_window_ids() -> std::vector<xcb_window_t>;
    auto get_window_pid(xcb_window_t window) -> unsigned int;
    auto set_parent_terminals() -> void;
    auto destroy() -> void;

private:
    void draw_image();
    void trigger_redraw();
    auto send_expose_event(xcb_window_t const& window, int x = 0, int y = 0) -> void;
    auto get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void;
    auto get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>;
    auto handle_events() -> void;

    xcb_connection_t *connection;
    xcb_screen_t *screen;

    Logging &logger;
    std::unique_ptr<Image> image;
    std::unique_ptr<std::thread> event_handler;
    std::unordered_map<int, std::unique_ptr<Terminal>> terminals;
};

#endif
