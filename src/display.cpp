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

#include <cmath>
#include <memory>
#include <xcb/xcb.h>
#include <nlohmann/json.hpp>

#include "display.hpp"
#include "tmux.hpp"
#include "os.hpp"
#include "util.hpp"
#include "free_delete.hpp"

using json = nlohmann::json;

Display::Display(Logging &logger):
logger(logger)
{
    // connect to server
    this->connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(this->connection)) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    // set screen
    this->screen = xcb_setup_roots_iterator(xcb_get_setup(this->connection)).data;

    this->set_parent_terminals();
    this->event_handler = std::make_unique<std::thread>([this] {
        this->handle_events();
    });
}

Display::~Display()
{
    // send custom event to first window
    if (this->terminals.size() != 0) {
        this->send_expose_event(terminals.begin()->second->get_window_id(), 69, 420);
    }
    // terminate event handler
    if (this->event_handler.get()) {
        this->event_handler->join();
    }

    xcb_disconnect(this->connection);
}

auto Display::action(std::string const& cmd) -> void
{
    json j;
    try {
        j = json::parse(cmd);
    } catch (json::parse_error const& e) {
        logger.log("There was an error parsing the command.");
        return;
    }
    logger.log(j.dump());
    if (j["action"] == "add") {
        for (const auto& [key, value]: terminals) {
            value->create_window(j["x"], j["y"], j["max_width"], j["max_height"]);
        }
        auto dimensions = terminals.begin()->second->get_window_dimensions();
        try {
            this->load_image(j["path"], dimensions.first, dimensions.second);
        } catch (const std::runtime_error& error) {
            logger.log(error.what());
        }
    } else {
        this->destroy_image();
    }
}

auto Display::set_parent_terminals() -> void
{
    std::vector<int> client_pids {os::get_pid()};
    std::unordered_map<unsigned int, xcb_window_t> pid_window_map;

    auto wid = os::getenv("WINDOWID");

    if (tmux::is_used()) {
        client_pids = tmux::get_client_pids().value();
        pid_window_map = this->get_pid_window_map();
    } else if (wid.has_value()) {
        // if WID exists prevent doing any calculations
        int pid = client_pids.front();
        this->terminals[pid] = std::make_unique<Terminal>
            (pid, std::stoi(wid.value()), this->connection, this->screen);
        return;
    }

    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_parent_pids(pid);
        for (const auto& ppid: ppids) {
            if (!pid_window_map.contains(ppid)) continue;
            this->terminals[ppid] = std::make_unique<Terminal>
                (ppid, pid_window_map[ppid], this->connection, this->screen);
        }
    }
}

auto Display::get_pid_window_map() -> std::unordered_map<unsigned int, xcb_window_t>
{
    std::unordered_map<unsigned int, xcb_window_t> res;
    for (auto window: this->get_server_window_ids()) {
        auto pid = this->get_window_pid(window);
        res[pid] = window;
    }
    return res;
}

void Display::destroy_image()
{
    for (auto const& [key, value]: terminals) {
        value->destroy_window();
    }
    this->image.reset();
    xcb_flush(this->connection);
}

void Display::load_image(std::string const& filename, int width, int height)
{
    this->image = std::make_unique<Image>
        (this->connection, this->screen, filename, width, height);
    this->trigger_redraw();
}

void Display::trigger_redraw()
{
    for (auto const& [key, value]: terminals) {
        this->send_expose_event(value->get_window_id());
    }
}

auto Display::send_expose_event(xcb_window_t const& window, int x, int y) -> void
{
    auto e = std::make_unique<xcb_expose_event_t>();
    e->response_type = XCB_EXPOSE;
    e->window = window;
    e->x = x;
    e->y = y;
    xcb_send_event(this->connection, false, window,
            XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(e.get()));
    xcb_flush(this->connection);
}

auto Display::get_server_window_ids() -> std::vector<xcb_window_t>
{
    auto cookie = xcb_query_tree_unchecked(this->connection, this->screen->root);
    std::vector<xcb_window_t> windows;
    get_server_window_ids_helper(windows, cookie);
    return windows;
}

auto Display::get_server_window_ids_helper(std::vector<xcb_window_t> &windows, xcb_query_tree_cookie_t cookie) -> void
{
    std::unique_ptr<xcb_query_tree_reply_t, free_delete> reply {
        xcb_query_tree_reply(this->connection, cookie, nullptr)
    };
    int num_children = xcb_query_tree_children_length(reply.get());

    if (!num_children) return;

    auto children = xcb_query_tree_children(reply.get());
    std::vector<xcb_query_tree_cookie_t> cookies;

    for (int i = 0; i < num_children; ++i) {
        auto child = children[i];
        bool is_complete_window = (
            util::window_has_property(this->connection, child, XCB_ATOM_WM_NAME, XCB_ATOM_STRING) &&
            util::window_has_property(this->connection, child, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING) &&
            util::window_has_property(this->connection, child, XCB_ATOM_WM_NORMAL_HINTS)
        );
        if (is_complete_window) windows.push_back(child);
        cookies.push_back(xcb_query_tree_unchecked(this->connection, child));
    }

    for (auto new_cookie: cookies) {
        this->get_server_window_ids_helper(windows, new_cookie);
    }
}

auto Display::get_window_pid(xcb_window_t window) -> unsigned int
{
    std::string atom_str = "_NET_WM_PID";

    auto atom_cookie = xcb_intern_atom_unchecked
        (this->connection, false, atom_str.size(), atom_str.c_str());
    auto atom_reply = std::unique_ptr<xcb_intern_atom_reply_t, free_delete> {
        xcb_intern_atom_reply(this->connection, atom_cookie, nullptr)
    };

    auto property_cookie = xcb_get_property_unchecked(
            this->connection, false, window, atom_reply->atom,
            XCB_ATOM_ANY, 0, 1);
    auto property_reply = std::unique_ptr<xcb_get_property_reply_t, free_delete> {
        xcb_get_property_reply(this->connection, property_cookie, nullptr),
    };

    return *reinterpret_cast<unsigned int*>(xcb_get_property_value(property_reply.get()));
}

auto Display::handle_events() -> void
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete> event {
            xcb_wait_for_event(this->connection)
        };
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                auto expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                if (expose->x == 69 && expose->y == 420) return;
                if (!this->image.get()) continue;
                this->image->draw(expose->window);
                break;
            }
            default: {
                break;
            }
        }
    }
}

