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

X11Canvas::X11Canvas()
{
    connection = std::shared_ptr<xcb_connection_t>(
        xcb_connect(nullptr, nullptr),
        x11_connection_deleter{}
    );
    if (xcb_connection_has_error(connection.get()) != 0) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    xutil = std::make_unique<X11Util>(connection);
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection.get())).data;
    logger = spdlog::get("X11");
    event_handler = std::thread([&] {
        logger->debug("Started event handler");
        handle_events();
    });

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
    const auto connfd = xcb_get_file_descriptor(connection.get());
    while (true) {
        const auto x11_event = os::wait_for_data_on_fd(connfd, 100);
        if (Application::stop_flag_.load()) {
            break;
        }
        if (!x11_event) {
            continue;
        }
        const auto event = unique_C_ptr<xcb_generic_event_t> {
            xcb_poll_for_event(connection.get())
        };
        if (event == nullptr) {
            continue;
        }
        switch (event->response_type & ~event_mask) {
            case XCB_EXPOSE: {
                const auto *expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                try {
                    windows.at(expose->window)->draw();
                } catch (const std::out_of_range& oor) {}
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

    if (tmux::is_used()) {
        auto tmp_pids = tmux::get_client_pids();
        if (tmp_pids.has_value()) {
            client_pids = tmp_pids.value();
        }
    } else if (wid.has_value()) {
        auto window_id = xcb_generate_id(connection.get());
        windows.insert({window_id, std::make_unique<Window>
                    (connection, screen, window_id, std::stoi(wid.value()), dimensions, *image)});
        logger->debug("Found WINDOWID={}", wid.value());
        return;
    }

    logger->debug("Need to manually find parent window");
    auto pid_window_map = xutil->get_pid_window_map();
    for (const auto& pid: client_pids) {
        // calculate a map with parent's pid and window id
        auto ppids = util::get_process_tree(pid);
        for (const auto& ppid: ppids) {
            auto search = pid_window_map.find(ppid);
            if (search == pid_window_map.end()) {
                continue;
            }
            auto window_id = xcb_generate_id(connection.get());
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
    windows.clear();
    image.reset();
    can_draw.store(true);
}
