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

#include <xcb/xproto.h>

X11Canvas::~X11Canvas()
{
    draw_thread.reset();
    windows.clear();
}

auto X11Canvas::draw() -> void
{
    if (!image->is_animated()) {
        for (const auto& window: windows) {
            window->draw();
        }
        return;
    }
    draw_thread = std::make_unique<std::jthread>([&] (std::stop_token token) {
        while (!token.stop_requested()) {
            for (const auto& window: windows) {
                window->draw();
            }
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

auto X11Canvas::init(const Dimensions& dimensions, std::shared_ptr<Image> image) -> void
{
    std::vector<int> client_pids {os::get_pid()};
    std::unordered_map<unsigned int, xcb_window_t> pid_window_map;
    this->image = image;

    auto wid = os::getenv("WINDOWID");

    if (tmux::is_used()) {
        client_pids = tmux::get_client_pids().value();
        pid_window_map = xutil.get_pid_window_map();
    } else if (wid.has_value()) {
        windows.push_back(std::make_unique<Window>
                    (std::stoi(wid.value()), dimensions, *image));
        return;
    }

    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_process_tree(pid);
        for (const auto& ppid: ppids) {
            auto search = pid_window_map.find(ppid);
            if (search == pid_window_map.end()) continue;
            windows.push_back(std::make_unique<Window>
                    (search->second, dimensions, *image));
        }
    }
}

auto X11Canvas::clear() -> void
{
    draw_thread.reset();
    windows.clear();
}
