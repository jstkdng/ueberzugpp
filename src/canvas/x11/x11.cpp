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

#include "x11.hpp"
#include "os.hpp"
#include "tmux.hpp"
#include "util.hpp"

#include <xcb/xcb.h>

X11Canvas::X11Canvas(const Terminal& terminal):
terminal(terminal)
{
    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
}

X11Canvas::~X11Canvas()
{
    windows.clear();
    xcb_disconnect(connection);
}

auto X11Canvas::draw() -> void
{
    for (const auto& window: windows) {
        window->draw();
    }
}

auto X11Canvas::init(const Dimensions& dimensions, std::shared_ptr<Image> image) -> void
{
    std::vector<ProcessInfo> client_pids {ProcessInfo(os::get_pid())};
    std::unordered_map<unsigned int, xcb_window_t> pid_window_map;
    this->image = image;

    auto wid = os::getenv("WINDOWID");

    if (tmux::is_used()) {
        client_pids = tmux::get_client_pids().value();
        pid_window_map = xutil.get_pid_window_map();
    } else if (wid.has_value()) {
        // if WID exists prevent doing any calculations
        auto proc = client_pids.front();
        windows.push_back(std::make_unique<Window>(
                    connection, std::stoi(wid.value()), screen,
                    dimensions, image));
        return;
    }

    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_parent_pids(pid);
        for (const auto& ppid: ppids) {
            auto search = pid_window_map.find(ppid.pid);
            if (search == pid_window_map.end()) continue;
            windows.push_back(std::make_unique<Window>(
                        connection, search->second, screen, dimensions, image));
        }
    }
}

auto X11Canvas::clear() -> void
{
    windows.clear();
}
