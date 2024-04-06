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

#include "wayland.hpp"
#include "application.hpp"
#include "image.hpp"
#include "os.hpp"
#include "util.hpp"

#ifdef ENABLE_OPENGL
#  include "window/waylandegl.hpp"
#  include <EGL/eglext.h>
#endif
#include "window/waylandshm.hpp"

constexpr struct wl_registry_listener registry_listener = {.global = WaylandCanvas::registry_handle_global,
                                                           .global_remove = [](auto...) { /*unused*/ }};

constexpr struct xdg_wm_base_listener xdg_wm_base_listener = {.ping = WaylandCanvas::xdg_wm_base_ping};

constexpr struct wl_output_listener wl_output_listener = {.geometry = [](auto...) { /*unused*/ },
                                                          .mode = [](auto...) { /*unused*/ },
                                                          .done = WaylandCanvas::output_done,
                                                          .scale = WaylandCanvas::output_scale,
                                                          .name = WaylandCanvas::output_name,
                                                          .description = [](auto...) { /*unused*/ }};

void WaylandCanvas::registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface,
                                           [[maybe_unused]] uint32_t version)
{
    const std::string_view interface_str{interface};
#ifdef WSL
    const uint32_t compositor_ver = 4;
    const uint32_t shm_ver = 1;
    const uint32_t xdg_base_ver = 1;
    const uint32_t output_ver = 3;
#else
    const uint32_t compositor_ver = 5;
    const uint32_t shm_ver = 1;
    const uint32_t xdg_base_ver = 2;
    const uint32_t output_ver = 4;
#endif

    auto *canvas = static_cast<WaylandCanvas *>(data);
    if (interface_str == wl_compositor_interface.name) {
        canvas->compositor = static_cast<struct wl_compositor *>(
            wl_registry_bind(registry, name, &wl_compositor_interface, compositor_ver));
    } else if (interface_str == wl_shm_interface.name) {
        canvas->wl_shm = static_cast<struct wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, shm_ver));
    } else if (interface_str == xdg_wm_base_interface.name) {
        canvas->xdg_base =
            static_cast<struct xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, xdg_base_ver));
        xdg_wm_base_add_listener(canvas->xdg_base, &xdg_wm_base_listener, canvas);
    } else if (interface_str == wl_output_interface.name) {
        auto *output =
            static_cast<struct wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, output_ver));
        wl_output_add_listener(output, &wl_output_listener, canvas);
    }
}

void WaylandCanvas::xdg_wm_base_ping([[maybe_unused]] void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandCanvas::output_name(void *data, [[maybe_unused]] struct wl_output *output, const char *name)
{
    auto *canvas = static_cast<WaylandCanvas *>(data);
    canvas->output_pair.first = name;
}

void WaylandCanvas::output_scale(void *data, [[maybe_unused]] struct wl_output *output, int32_t scale)
{
    auto *canvas = static_cast<WaylandCanvas *>(data);
    canvas->output_pair.second = scale;
}

void WaylandCanvas::output_done(void *data, [[maybe_unused]] struct wl_output *output)
{
    auto *canvas = static_cast<WaylandCanvas *>(data);
    const auto active_output = canvas->config->get_focused_output_name();
    if (active_output == canvas->output_pair.first) {
        canvas->flags->scale_factor = canvas->output_pair.second;
        canvas->flags->needs_scaling = canvas->output_pair.second > 1;
    }
    canvas->output_info.insert(canvas->output_pair);
}

WaylandCanvas::WaylandCanvas()
    : display(wl_display_connect(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Failed to connect to wayland display.");
    }
    logger = spdlog::get("wayland");
    flags = Flags::instance();
    config = WaylandConfig::get();
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);
    event_handler = std::thread(&WaylandCanvas::handle_events, this);

#ifdef ENABLE_OPENGL
    if (flags->use_opengl) {
        try {
            egl = std::make_unique<EGLUtil<struct wl_display, struct wl_egl_window>>(EGL_PLATFORM_WAYLAND_EXT, display);
        } catch (const std::runtime_error &) {
            egl_available = false;
        }
    } else {
        egl_available = false;
    }
#endif

    logger->info("Canvas created");
}

void WaylandCanvas::show()
{
    for (const auto &[key, value] : windows) {
        value->show();
    }
}

void WaylandCanvas::hide()
{
    for (const auto &[key, value] : windows) {
        value->hide();
    }
}

WaylandCanvas::~WaylandCanvas()
{
    windows.clear();
    if (event_handler.joinable()) {
        event_handler.join();
    }

#ifdef ENABLE_OPENGL
    egl.reset();
#endif

    if (wl_shm != nullptr) {
        wl_shm_destroy(wl_shm);
    }
    if (compositor != nullptr) {
        wl_compositor_destroy(compositor);
    }
    if (registry != nullptr) {
        wl_registry_destroy(registry);
    }
    wl_display_disconnect(display);
}

void WaylandCanvas::add_image(const std::string &identifier, std::unique_ptr<Image> new_image)
{
    std::shared_ptr<WaylandWindow> window;
#ifdef ENABLE_OPENGL
    if (egl_available) {
        try {
            window = std::make_shared<WaylandEglWindow>(compositor, xdg_base, egl.get(), std::move(new_image),
                                                        config.get(), &xdg_agg);
        } catch (const std::runtime_error &err) {
            return;
        }
    }
#endif
    if (window == nullptr) {
        window = std::make_shared<WaylandShmWindow>(this, std::move(new_image), &xdg_agg, config.get());
    }

    window->finish_init();
    windows.insert_or_assign(identifier, std::move(window));
}

void WaylandCanvas::handle_events()
{
    const auto wl_fd = wl_display_get_fd(display);
    bool in_event = false;

    while (!Application::stop_flag) {
        // prepare to read wayland events
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }
        wl_display_flush(display);

        try {
            constexpr int waitms = 100;
            in_event = os::wait_for_data_on_fd(wl_fd, waitms);
        } catch (const std::system_error &err) {
            Application::stop_flag = true;
            break;
        }

        if (in_event) {
            wl_display_read_events(display);
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }
    }
}

void WaylandCanvas::remove_image(const std::string &identifier)
{
    windows.erase(identifier);
    wl_display_flush(display);
}
