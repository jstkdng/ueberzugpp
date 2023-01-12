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
#include <xcb/xproto.h>
#include <xcb/res.h>

#include "display.hpp"

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
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_disconnect(this->connection);
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

auto Display::get_window_pid() -> int
{
    int num_specs = 1;
    std::unique_ptr<xcb_res_client_id_spec_t> client_specs = std::make_unique<xcb_res_client_id_spec_t>();
    client_specs->client = this->window;
    client_specs->mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;
    /*
    auto cookie = xcb_res_query_client_ids_unchecked
        (this->connection, num_specs, client_specs.get());
    std::unique_ptr<xcb_res_query_client_ids_reply_t, decltype(&free)> reply
        (xcb_res_query_client_ids_reply(this->connection, cookie, nullptr), free);
    auto clients = xcb_res_query_clients_clients(reply.get());
    std::cout << reply->num_ids << std::endl;
    auto iter = xcb_res_query_client_ids_ids_iterator(reply.get());
    for (int i = 0; i < xcb_res_query_client_ids_ids_length(reply.get()); ++i) {
        std::cout << iter.data->spec.client << std::endl;
        xcb_res_client_id_value_next(&iter);
    }*/
    return 0;
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

