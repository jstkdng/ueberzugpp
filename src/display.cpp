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
#include "logging.hpp"

using json = nlohmann::json;

Display::Display()
{
    // connect to server
    this->connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(this->connection)) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    // set screen
    this->screen = xcb_setup_roots_iterator(xcb_get_setup(this->connection)).data;

    this->set_parent_terminals();
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
        logger << "There was an error parsing the command." << std::endl;
        return;
    }
    logger << "=== Command received:\n" << j.dump() << std::endl;
    if (j["action"] == "add") {
        for (const auto& [key, value]: this->terminals) {
            value->create_window(j["x"], j["y"], j["max_width"], j["max_height"]);
        }
        // spawn event handler once windows exist
        if (!this->event_handler.get()) {
        }
        auto dimensions = terminals.begin()->second->get_window_dimensions();
        try {
            this->load_image(j["path"], dimensions.first, dimensions.second);
        } catch (const std::runtime_error& error) {
            logger << error.what() << std::endl;;
        }
    } else {
        this->destroy_image();
    }
}

auto Display::set_parent_terminals() -> void
{
    std::vector<ProcessInfo> client_pids {ProcessInfo(os::get_pid())};
    std::unordered_map<unsigned int, xcb_window_t> pid_window_map;

    auto wid = os::getenv("WINDOWID");

    if (tmux::is_used()) {
        client_pids = tmux::get_client_pids().value();
        pid_window_map = this->get_pid_window_map();
    } else if (wid.has_value()) {
        // if WID exists prevent doing any calculations
        auto proc = client_pids.front();
        this->terminals[proc.pid] = std::make_unique<Terminal>
            (proc, std::stoi(wid.value()), this->connection, this->screen);
        return;
    }

    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_parent_pids(pid);
        for (const auto& ppid: ppids) {
            if (!pid_window_map.contains(ppid.pid)) continue;
            this->terminals[ppid.pid] = std::make_unique<Terminal>
                (ppid, pid_window_map[ppid.pid], this->connection, this->screen);
        }
    }
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
    this->image = std::make_unique<ImageL>
        (this->connection, this->screen, filename, width, height);
    this->trigger_redraw();
}

void Display::trigger_redraw()
{
    for (auto const& [key, value]: terminals) {
        this->send_expose_event(value->get_window_id());
    }
}




