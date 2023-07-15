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
#include "application.hpp"
#include "flags.hpp"

#include <spdlog/spdlog.h>

#ifdef ENABLE_OPENGL
#   include <EGL/eglext.h>
#   include "window/x11egl.hpp"
#endif
#include "window/x11.hpp"

X11Canvas::X11Canvas():
connection(xcb_connect(nullptr, &screen_num))
{
    if (xcb_connection_has_error(connection) > 0) {
        throw std::runtime_error("Can't connect to X11 server");
    }

    const auto* xcb_setup = xcb_get_setup(connection);
    auto iter = xcb_setup_roots_iterator(xcb_setup);
    for (int i = 0; i < screen_num; ++i) {
        xcb_screen_next(&iter);
    }
    screen = iter.data;

#ifdef ENABLE_XCB_ERRORS
    xcb_errors_context_new(connection, &err_ctx);
#endif

#ifdef ENABLE_OPENGL
    egl_display = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, connection, nullptr);
    if (egl_display != EGL_NO_DISPLAY) {
        auto eglres = eglInitialize(egl_display, nullptr, nullptr);
        if (eglres == EGL_TRUE) {
            eglres = eglBindAPI(EGL_OPENGL_API);
            egl_available = eglres == EGL_TRUE;
        }
    }
#endif

    xutil = std::make_unique<X11Util>(connection);
    logger = spdlog::get("X11");
    flags = Flags::instance();
    event_handler = std::thread([this] {
        logger->debug("Started event handler");
        handle_events();
        logger->debug("Stopped event handler");
    });
    logger->info("Canvas created");
}

X11Canvas::~X11Canvas()
{
    draw_threads.clear();
    if (event_handler.joinable()) {
        event_handler.join();
    }

#ifdef ENABLE_OPENGL
    if (egl_available) {
        eglTerminate(egl_display);
    }
#endif

#ifdef ENABLE_XCB_ERRORS
    xcb_errors_context_free(err_ctx);
#endif

    xcb_disconnect(connection);
}

void X11Canvas::draw(const std::string& identifier)
{
    if (!images.at(identifier)->is_animated()) {
        for (const auto& [wid, window]: image_windows.at(identifier)) {
            window->generate_frame();
        }
        return;
    }

    draw_threads.insert_or_assign(identifier,
        std::jthread([this, identifier] (const std::stop_token& stoken) {
            const auto image = images.at(identifier);
            const auto wins = image_windows.at(identifier);
            while (!stoken.stop_requested()) {
                for (const auto& [wid, window]: wins) {
                    window->generate_frame();
                }
                image->next_frame();
                std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
            }
    }));
}

void X11Canvas::show()
{
    const std::scoped_lock lock {windows_mutex};
    for (const auto& [wid, window]: windows) {
        window->show();
    }
}

void X11Canvas::hide()
{
    const std::scoped_lock lock {windows_mutex};
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
        const std::scoped_lock lock {windows_mutex};
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
                        const auto window = windows.at(expose->window);
                        window->draw();
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

void X11Canvas::add_image(const std::string& identifier, std::unique_ptr<Image> new_image)
{
    remove_image(identifier);

    logger->debug("Initializing canvas");
    images.insert({identifier, std::move(new_image)});
    image_windows.insert({identifier, {}});

    const auto image = images.at(identifier);
    const auto dims = image->dimensions();
    std::unordered_set<xcb_window_t> parent_ids {dims.terminal.x11_wid};
    get_tmux_window_ids(parent_ids);

    std::ranges::for_each(std::as_const(parent_ids), [this, &identifier, &image] (xcb_window_t parent) {
        const auto window_id = xcb_generate_id(connection);
        std::shared_ptr<Window> window;
#ifdef ENABLE_OPENGL
        if (egl_available && flags->use_opengl) {
            window = std::make_shared<X11EGLWindow>(connection, screen, window_id, parent, egl_display, image);
        } else {
            window = std::make_shared<X11Window>(connection, screen, window_id, parent, image);
        }
#else
        window = std::make_shared<X11Window>(connection, screen, window_id, parent, image);
#endif
        windows.insert({window_id, window});
        image_windows.at(identifier).insert({window_id, window});
        window->show();
    });

    draw(identifier);
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
#ifdef ENABLE_XCB_ERRORS
    const char *extension = nullptr;
    const char *major = xcb_errors_get_name_for_major_code(err_ctx, err->major_code);
    const char *minor = xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code);
    const char *error = xcb_errors_get_name_for_error(err_ctx, err->error_code, &extension);
    logger->error("XCB: {}:{}, {}:{}, resource {} sequence {}",
           error, extension != nullptr ? extension : "no_extension",
           major, minor != nullptr ? minor : "no_minor",
           err->resource_id, err->sequence);
#else
    logger->error("XCB: resource {} sequence {}", err->resource_id, err->sequence);
#endif
}

void X11Canvas::remove_image(const std::string& identifier)
{
    draw_threads.erase(identifier);
    images.erase(identifier);

    const std::scoped_lock lock {windows_mutex};
    const auto old_windows = image_windows.extract(identifier);
    if (old_windows.empty()) {
        return;
    }
    for (const auto& [key, value]: old_windows.mapped()) {
        windows.erase(key);
    }
}
