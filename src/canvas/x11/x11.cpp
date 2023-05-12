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


X11Canvas::X11Canvas(std::mutex& img_lock):
img_lock(img_lock),
connection(xcb_connect(nullptr, nullptr))
{
    if (xcb_connection_has_error(connection) != 0) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    logger = spdlog::get("X11");
    logger->debug("Created X11 canvas");
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
            std::unique_lock lock {img_lock, std::try_to_lock};
            if (!lock.owns_lock()) {
                return;
            }
            for (const auto& [wid, window]: windows) {
                window->generate_frame();
            }
            image->next_frame();
            lock.unlock();
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

auto X11Canvas::handle_events() -> void
{
    discard_leftover_events();
    while (true) {
        auto event = unique_C_ptr<xcb_generic_event_t> {
            xcb_wait_for_event(this->connection)
        };
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                auto *expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                if (expose->x == 69 && expose->y == 420) {
                    return;
                }
                try {
                    std::scoped_lock lock (windows_mutex);
                    windows.at(expose->window)->draw();
                } catch (const std::out_of_range& ex) {}
                break;
            }
            default: {
                break;
            }
        }
    }
}

auto X11Canvas::init(const Dimensions& dimensions, std::shared_ptr<Image> image) -> void
{
    logger->debug("Initializing canvas");
    std::vector<int> client_pids;
    if (Application::parent_pid != getppid()) {
        logger->debug("Running in daemon mode");
        client_pids.push_back(Application::parent_pid);
    } else {
        client_pids.push_back(os::get_pid());
    }
    this->image = image;

    auto wid = os::getenv("WINDOWID");

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

auto X11Canvas::clear() -> void
{
    {
        std::scoped_lock lock (windows_mutex);
        windows.clear();
    }

    if (event_handler.joinable()) {
        event_handler.join();
    }
    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    can_draw.store(true);
}

void X11Canvas::discard_leftover_events()
{
    auto event = unique_C_ptr<xcb_generic_event_t> {
        xcb_poll_for_event(connection)
    };
    while (event.get() != nullptr) {
        event.reset(xcb_poll_for_event(connection));
    }
}
