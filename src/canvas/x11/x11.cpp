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

#include <X11/Xlib-xcb.h>

X11Canvas::X11Canvas():
display(XOpenDisplay(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Can't open X11 display");
    }
    default_screen = XDefaultScreen(display);
    connection = XGetXCBConnection(display);
    if (connection == nullptr) {
        throw std::runtime_error("Can't get xcb connection from display");
    }
    XSetEventQueueOwner(display, XCBOwnsEventQueue);
    xcb_errors_context_new(connection, &err_ctx);
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
    for(int screen_num = default_screen; screen_iter.rem > 0 && screen_num > 0; --screen_num) {
        xcb_screen_next(&screen_iter);
    }
    screen = screen_iter.data;

#ifdef ENABLE_OPENGL
    egl_display = eglGetDisplay(reinterpret_cast<NativeDisplayType>(display));
    eglInitialize(egl_display, nullptr, nullptr);
#endif

    xutil = std::make_unique<X11Util>(connection);
    logger = spdlog::get("X11");
    event_handler = std::thread([&] {
        logger->debug("Started event handler");
        handle_events();
        logger->debug("Stopped event handler");
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
#ifdef ENABLE_OPENGL
    eglTerminate(egl_display);
#endif
    xcb_errors_context_free(err_ctx);
    XCloseDisplay(display);
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
    const int event_mask = 0x80;
    const int waitms = 100;
    const int connfd = xcb_get_file_descriptor(connection);

    while (true) {
        const bool status = os::wait_for_data_on_fd(connfd, waitms);
        if (Application::stop_flag_.load()) {
            break;
        }
        if (!status) {
            continue;
        }
        std::scoped_lock lock {windows_mutex};
        auto event = unique_C_ptr<xcb_generic_event_t> {
            xcb_poll_for_event(connection)
        };
        while (event) {
            const int real_event = event->response_type & ~event_mask;
            switch (real_event) {
                case 0: {
                    const auto *err = reinterpret_cast<xcb_generic_error_t*>(event.get());
                    print_xcb_error(err);
                    break;
                }
                case XCB_EXPOSE: {
                    const auto *expose = reinterpret_cast<xcb_expose_event_t*>(event.get());
                    try {
                        windows.at(expose->window)->draw();
                        logger->debug("Received expose event for window {}", expose->window);
                    } catch (const std::out_of_range& oor) {
                        logger->debug("Discarding expose event for window {}", expose->window);
                    }
                    break;
                }
                default: {
                    logger->debug("Received unknown event {}", real_event);
                    break;
                }
            }
            event.reset(xcb_poll_for_event(connection));
        }
    }
}

void X11Canvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    logger->debug("Initializing canvas");
    image = std::move(new_image);

    std::unordered_set<xcb_window_t> parent_ids {dimensions.terminal.x11_wid};
    get_tmux_window_ids(parent_ids);

    std::ranges::for_each(std::as_const(parent_ids), [&] (xcb_window_t parent) {
        const auto window_id = xcb_generate_id(connection);
        windows.insert({window_id, std::make_unique<X11Window>
                (connection, screen, window_id, parent, dimensions, *image)});
    });
}

void X11Canvas::get_tmux_window_ids(std::unordered_set<xcb_window_t>& windows)
{
    const auto pids = tmux::get_client_pids();
    if (!pids.has_value()) {
        return;
    }
    const auto pid_window_map = xutil->get_pid_window_map();
    for (const auto pid: pids.value()) {
        const auto ppids = util::get_process_tree(pid);
        for (const auto ppid: ppids) {
            const auto win = pid_window_map.find(ppid);
            if (win == pid_window_map.end()) {
                continue;
            }
            windows.insert(win->second);
        }
    }
}

void X11Canvas::print_xcb_error(const xcb_generic_error_t* err)
{
    const char *extension = nullptr;
    const char *major = xcb_errors_get_name_for_major_code(err_ctx, err->major_code);
    const char *minor = xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code);
    const char *error = xcb_errors_get_name_for_error(err_ctx, err->error_code, &extension);
    logger->error("XCB: {}:{}, {}:{}, resource {} sequence {}",
           error, extension != nullptr ? extension : "no_extension",
           major, minor != nullptr ? minor : "no_minor",
           err->resource_id, err->sequence);
}

void X11Canvas::clear()
{
    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    {
        std::scoped_lock lock {windows_mutex};
        windows.clear();
    }
    image.reset();
    can_draw.store(true);
}
