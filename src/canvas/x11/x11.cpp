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
#include "util/ptr.hpp"
#include "application.hpp"

#include <xcb/xproto.h>
#include <ranges>


X11Canvas::X11Canvas():
connection(xcb_connect(nullptr, nullptr))
{
    if (xcb_connection_has_error(connection) != 0) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    logger = spdlog::get("X11");
    logger->info("Canvas created");
}

X11Canvas::~X11Canvas()
{
    if (event_handler.joinable()) {
        event_handler.join();
    }
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    xcb_disconnect(connection);
}

void X11Canvas::draw()
{
    if (!image->is_animated()) {
        for (const auto& [wid, window]: windows) {
            window->generate_frame();
        }
        return;
    }
    draw_thread = std::thread([&]  {
        while (can_draw.load()) {
            for (const auto& [wid, window]: windows) {
                window->generate_frame();
            }
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

void X11Canvas::toggle()
{
    for (const auto& [wid, window]: windows) {
        window->toggle();
    }
}

void X11Canvas::show()
{
    for (const auto& [wid, window]: windows) {
        window->show();
    }
}

void X11Canvas::hide()
{
    for (const auto& [wid, window]: windows) {
        window->hide();
    }
}

void X11Canvas::handle_events()
{
    const auto event_mask = 0x80;
    while (true) {
        const auto event = unique_C_ptr<xcb_generic_event_t> {
            xcb_wait_for_event(this->connection)
        };
        switch (event->response_type & ~event_mask) {
            case XCB_EXPOSE: {
                const auto *expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                if (expose->x == MAGIC_EXIT_NUM1 && expose->y == MAGIC_EXIT_NUM2) {
                    windows.erase(expose->window);
                    if (windows.empty()) {
                        return;
                    }
                    continue;
                }
                windows.at(expose->window)->draw();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void X11Canvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    logger->debug("Initializing canvas");
    std::vector<int> client_pids;
    if (Application::parent_pid_ != os::get_ppid()) {
        logger->debug("Running in daemon mode");
        client_pids.push_back(Application::parent_pid_);
    } else {
        client_pids.push_back(os::get_pid());
    }
    image = std::move(new_image);

    const auto wid = os::getenv("WINDOWID");

    event_handler = std::thread([&] {
        logger->debug("Started event handler");
        handle_events();
    });

    if (tmux::is_used()) {
        auto tmp_pids = tmux::get_client_pids();
        if (tmp_pids.has_value()) {
            client_pids = tmp_pids.value();
        }
    } else if (wid.has_value()) {
        auto window_id = xcb_generate_id(connection);
        windows.insert({window_id, std::make_unique<Window>
                    (connection, screen, window_id, std::stoi(wid.value()), dimensions, *image)});
        logger->debug("Found WINDOWID={}", wid.value());
        return;
    }

    logger->debug("Need to manually find parent window");
    auto pid_window_map = xutil.get_pid_window_map();
    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_process_tree(pid);
        for (const auto& ppid: ppids) {
            auto search = pid_window_map.find(ppid);
            if (search == pid_window_map.end()) {
                continue;
            }
            auto window_id = xcb_generate_id(connection);
            windows.insert({window_id, std::make_unique<Window>
                    (connection, screen, window_id, search->second, dimensions, *image)});
        }
    }
}

void X11Canvas::clear()
{
    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    for (const auto& [key, value]: windows) {
        value->terminate_event_handler();
    }
    if (event_handler.joinable()) {
        event_handler.join();
    }
    image.reset();
    can_draw.store(true);
}
