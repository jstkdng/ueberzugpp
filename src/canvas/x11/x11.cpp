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
#include "application.hpp"
#include "flags.hpp"
#include "os.hpp"
#include "tmux.hpp"
#include "util.hpp"

#include <string_view>

#include <range/v3/all.hpp>

#ifdef ENABLE_OPENGL
#  include "window/x11egl.hpp"
#  include <EGL/eglext.h>
#endif
#include "window/x11.hpp"

X11Canvas::X11Canvas()
    : connection(xcb_connect(nullptr, nullptr))
{
    if (xcb_connection_has_error(connection) > 0) {
        throw std::runtime_error("Can't connect to X11 server");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

#ifdef ENABLE_XCB_ERRORS
    xcb_errors_context_new(connection, &err_ctx);
#endif

    flags = Flags::instance();

#ifdef ENABLE_OPENGL
    if (flags->use_opengl) {
        try {
            egl = std::make_unique<EGLUtil<xcb_connection_t, xcb_window_t>>(EGL_PLATFORM_XCB_EXT, connection);
        } catch (const std::runtime_error &err) {
            egl_available = false;
        }
    } else {
        egl_available = false;
    }
#endif

    xutil = std::make_unique<X11Util>(connection);
    logger = spdlog::get("X11");
    event_handler = std::thread(&X11Canvas::handle_events, this);
    logger->info("Canvas created");
}

X11Canvas::~X11Canvas()
{
    draw_threads.clear();
    windows.clear();
    image_windows.clear();

    if (event_handler.joinable()) {
        event_handler.join();
    }

#ifdef ENABLE_XCB_ERRORS
    xcb_errors_context_free(err_ctx);
#endif

    xcb_disconnect(connection);
}

void X11Canvas::draw(const std::string &identifier)
{
    if (!images.at(identifier)->is_animated()) {
        for (const auto &[wid, window] : image_windows.at(identifier)) {
            window->generate_frame();
        }
        return;
    }

    draw_threads.insert_or_assign(identifier, std::jthread([this, identifier](const std::stop_token &stoken) {
                                      const auto image = images.at(identifier);
                                      const auto wins = image_windows.at(identifier);
                                      while (!stoken.stop_requested()) {
                                          for (const auto &[wid, window] : wins) {
                                              window->generate_frame();
                                          }
                                          image->next_frame();
                                          std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
                                      }
                                  }));
}

void X11Canvas::show()
{
    const std::scoped_lock lock{windows_mutex};
    for (const auto &[wid, window] : windows) {
        window->show();
    }
}

void X11Canvas::hide()
{
    const std::scoped_lock lock{windows_mutex};
    for (const auto &[wid, window] : windows) {
        window->hide();
    }
}

void X11Canvas::handle_events()
{
    const int event_mask = 0x80;
    const int waitms = 100;
    const int connfd = xcb_get_file_descriptor(connection);
    bool status = false;

    while (!Application::stop_flag) {
        try {
            status = os::wait_for_data_on_fd(connfd, waitms);
        } catch (const std::system_error &err) {
            Application::stop_flag = true;
            break;
        }

        if (!status) {
            continue;
        }

        const std::scoped_lock lock{windows_mutex};
        auto event = unique_C_ptr<xcb_generic_event_t>{xcb_poll_for_event(connection)};
        while (event) {
            const int real_event = event->response_type & ~event_mask;
            switch (real_event) {
                case 0: {
                    const auto *err = reinterpret_cast<xcb_generic_error_t *>(event.get());
                    print_xcb_error(err);
                    break;
                }
                case XCB_EXPOSE: {
                    const auto *expose = reinterpret_cast<xcb_expose_event_t *>(event.get());
                    try {
                        logger->debug("Received expose event for window {}", expose->window);
                        const auto window = windows.at(expose->window);
                        window->draw();
                    } catch (const std::out_of_range &oor) {
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

void X11Canvas::add_image(const std::string &identifier, std::unique_ptr<Image> new_image)
{
    remove_image(identifier);

    logger->debug("Initializing canvas");
    images.insert({identifier, std::move(new_image)});
    image_windows.insert({identifier, {}});

    const auto image = images.at(identifier);
    const auto dims = image->dimensions();
    std::unordered_set<xcb_window_t> parent_ids{dims.terminal->x11_wid};
    get_tmux_window_ids(parent_ids);

    ranges::for_each(parent_ids, [this, &identifier, &image](xcb_window_t parent) {
        const auto window_id = xcb_generate_id(connection);
        std::shared_ptr<Window> window;
#ifdef ENABLE_OPENGL
        if (egl_available) {
            try {
                window = std::make_shared<X11EGLWindow>(connection, screen, window_id, parent, egl.get(), image);
            } catch (const std::runtime_error &err) {
                return;
            }
        }
#endif
        if (window == nullptr) {
            window = std::make_shared<X11Window>(connection, screen, window_id, parent, image);
        }
        windows.insert({window_id, window});
        image_windows.at(identifier).insert({window_id, window});
        if (tmux::is_used() && !tmux::is_window_focused()) {
          window->hide();
          return;
        }
        window->show();
    });

    draw(identifier);
}

void X11Canvas::get_tmux_window_ids(std::unordered_set<xcb_window_t> &windows)
{
    const auto pids = tmux::get_client_pids();
    if (!pids.has_value()) {
        return;
    }
    const auto pid_window_map = xutil->get_pid_window_map();
    for (const auto pid : pids.value()) {
        const auto ppids = util::get_process_tree(pid);
        for (const auto ppid : ppids) {
            const auto win = pid_window_map.find(ppid);
            if (win == pid_window_map.end()) {
                continue;
            }
            windows.insert(win->second);
            break; // prevent multiple windows being created
        }
    }
}

void X11Canvas::print_xcb_error(const xcb_generic_error_t *err)
{
#ifdef ENABLE_XCB_ERRORS
    const char *extension = nullptr;
    const char *major = xcb_errors_get_name_for_major_code(err_ctx, err->major_code);
    const char *minor = xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code);
    const char *error = xcb_errors_get_name_for_error(err_ctx, err->error_code, &extension);

    const std::string_view ext_str = extension != nullptr ? extension : "no_extension";
    const std::string_view minor_str = minor != nullptr ? minor : "no_minor";
    logger->error("XCB: {}:{}, {}:{}, resource {} sequence {}", error, ext_str, major, minor_str, err->resource_id,
                  err->sequence);
#else
    logger->error("XCB: resource {} sequence {}", err->resource_id, err->sequence);
#endif
}

void X11Canvas::remove_image(const std::string &identifier)
{
    draw_threads.erase(identifier);
    images.erase(identifier);

    const std::scoped_lock lock{windows_mutex};
    const auto old_windows = image_windows.extract(identifier);
    if (old_windows.empty()) {
        return;
    }
    for (const auto &[key, value] : old_windows.mapped()) {
        windows.erase(key);
    }
}
