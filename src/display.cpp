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

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <xcb/xcb.h>

#include "display.hpp"
#include "tmux.hpp"
#include "os.hpp"
#include "process_info.hpp"

Display::Display(Logging &logger):
logger(logger)
{
    int screen_num;
    this->connection = xcb_connect(NULL, &screen_num);
    // set screen
    const xcb_setup_t *setup = xcb_get_setup(this->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screen_num; ++i) {
        xcb_screen_next(&iter);
    }
    this->screen = iter.data;
    // create image
    this->image = std::make_unique<Image>(this->connection, this->screen);
    this->get_parent_terminals();
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_disconnect(this->connection);
}

auto Display::get_parent_terminals() -> void
{
    std::vector<int> client_pids {os::get_pid()};
    if (tmux::is_used()) client_pids = tmux::get_client_pids().value();

    auto pid_window_map = this->get_pid_window_map();
    for (const auto& pid: client_pids) {
        auto ppids = this->get_parent_pids(pid);
        std::unordered_map<int, xcb_window_t> ppid_window_id_map;
        for (const auto& ppid: ppids) {
            if (pid_window_map.contains(ppid)) {
                ppid_window_id_map[ppid] = pid_window_map[ppid];
            }
        }
        for (const auto& [key, value]: ppid_window_id_map) {
            std::cout << "key: " << key << " value: " << value << std::endl;
        }
    }
}

auto Display::get_parent_pids(int pid) -> std::vector<int>
{
    ProcessInfo proc(pid);
    std::vector<int> ppids;
    while (proc.pid != 1) {
        ppids.push_back(proc.ppid);
        proc = ProcessInfo(proc.ppid);
    }
    return ppids;
}

auto Display::get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>
{
    std::unordered_map<unsigned int, xcb_window_t> res;
    for (auto window: this->get_server_window_ids()) {
        auto pid = this->get_window_pid(window);
        if (pid) res[pid] = window;
    }
    return res;
}

void Display::destroy_image()
{
    this->image->destroy();
    xcb_clear_area(this->connection, false, this->window, 0, 0, 0, 0);
    //xcb_unmap_window(this->connection, this->window);
    xcb_flush(this->connection);
}

void Display::load_image(std::string filename)
{
    //xcb_map_window(this->connection, this->window);
    this->image->load(filename);
    this->trigger_redraw();
}

void Display::trigger_redraw()
{
    xcb_expose_event_t *e = static_cast<xcb_expose_event_t*>(calloc(1, sizeof(xcb_expose_event_t)));
    e->response_type = XCB_EXPOSE;
    e->window = this->window;
    xcb_send_event(this->connection, false, this->window, XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(e));
    xcb_flush(this->connection);
}


auto Display::get_server_window_ids() -> std::vector<xcb_window_t>
{
    auto cookie = xcb_query_tree(this->connection, this->screen->root);
    std::vector<xcb_window_t> windows;
    get_server_window_ids_helper(windows, cookie);
    return windows;
}

auto Display::get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void
{
    std::unique_ptr<xcb_query_tree_reply_t, decltype(&free)> reply(
        xcb_query_tree_reply(this->connection, cookie, NULL), free);
    int num_children = xcb_query_tree_children_length(reply.get());

    if (!num_children) return;

    auto children = xcb_query_tree_children(reply.get());
    std::vector<xcb_query_tree_cookie_t> cookies;

    for (int i = 0; i < num_children; ++i) {
        windows.push_back(children[i]);
        cookies.push_back(xcb_query_tree(this->connection, children[i]));
    }

    for (auto new_cookie: cookies) {
        this->get_server_window_ids_helper(windows, new_cookie);
    }
}

auto Display::get_window_pid(xcb_window_t window) -> unsigned int
{
    std::string atom_str = "_NET_WM_PID";

    xcb_generic_error_t *err = nullptr;
    auto atom_cookie = xcb_intern_atom
        (this->connection, true, atom_str.size(), atom_str.c_str());
    std::unique_ptr<xcb_intern_atom_reply_t, decltype(&free)> atom_reply {
        xcb_intern_atom_reply(this->connection, atom_cookie, &err),
        &free
    };

    auto property_cookie = xcb_get_property_unchecked(
            this->connection, false, window, atom_reply->atom,
            XCB_ATOM_CARDINAL, 0, 1);
    std::unique_ptr<xcb_get_property_reply_t, decltype(&free)> property_reply {
        xcb_get_property_reply(this->connection, property_cookie, &err),
        &free
    };

    if (err) {
        free(err);
        return 0;
    }

    return *reinterpret_cast<unsigned int*>(xcb_get_property_value(property_reply.get()));
}

void Display::create_window()
{
    unsigned int value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    unsigned int value_list[4] = {
        this->screen->black_pixel,
        this->screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE,
        this->screen->default_colormap
    };

    xcb_window_t wid = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
            this->screen->root_depth,
            wid,
            this->screen->root,
            800, 50,
            500, 500,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);

    this->window = wid;
    xcb_map_window(this->connection, this->window);
    xcb_flush(this->connection);
}

std::thread Display::spawn_event_handler()
{
    return std::thread([this] {
        this->handle_events();
    });
}

void Display::handle_events()
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, decltype(&free)>
            event (xcb_wait_for_event(this->connection), free);
        auto response = event->response_type & ~0x80;
        switch (response) {
            case XCB_EXPOSE: {
                this->image->draw(this->window);
                break;
            }
            default: {
                break;
            }
        }
    }
}

