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
#include "image.hpp"
#include "os.hpp"
#include "util.hpp"

#include <spdlog/spdlog.h>

#ifdef ENABLE_OPENGL
#  include "window/waylandegl.hpp"
#  include <EGL/eglext.h>
#endif
#include "window/waylandshm.hpp"

constexpr struct wl_registry_listener registry_listener = {.global = WaylandCanvas::registry_handle_global,
                                                           .global_remove = [](auto...) {}};

constexpr struct xdg_wm_base_listener xdg_wm_base_listener = {.ping = WaylandCanvas::xdg_wm_base_ping};

constexpr struct wl_output_listener wl_output_listener = {.geometry = [](auto...) {},
                                                          .mode = [](auto...) {},
                                                          .done = WaylandCanvas::output_done,
                                                          .scale = WaylandCanvas::output_scale,
                                                          .name = WaylandCanvas::output_name,
                                                          .description = [](auto...) {}};

void WaylandCanvas::registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface,
                                           uint32_t version)
{
    const std::string_view interface_str{interface};
    auto *canvas = reinterpret_cast<WaylandCanvas *>(data);
    if (interface_str == wl_compositor_interface.name) {
        canvas->compositor =
            static_cast<struct wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, version));
    } else if (interface_str == wl_shm_interface.name) {
        canvas->wl_shm = static_cast<struct wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, version));
    } else if (interface_str == xdg_wm_base_interface.name) {
        canvas->xdg_base =
            static_cast<struct xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, version));
        xdg_wm_base_add_listener(canvas->xdg_base, &xdg_wm_base_listener, canvas);
    } else if (interface_str == wl_output_interface.name) {
        auto *output = static_cast<struct wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, version));
        wl_output_add_listener(output, &wl_output_listener, canvas);
    }
}

void WaylandCanvas::xdg_wm_base_ping([[maybe_unused]] void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandCanvas::output_name(void *data, [[maybe_unused]] struct wl_output *output, const char *name)
{
    auto *canvas = reinterpret_cast<WaylandCanvas *>(data);
    canvas->output_pair.first = name;
}

void WaylandCanvas::output_scale(void *data, [[maybe_unused]] struct wl_output *output, int32_t scale)
{
    auto *canvas = reinterpret_cast<WaylandCanvas *>(data);
    canvas->output_pair.second = scale;
}

void WaylandCanvas::output_done(void *data, [[maybe_unused]] struct wl_output *output)
{
    auto *canvas = reinterpret_cast<WaylandCanvas *>(data);
    canvas->output_info.emplace(canvas->output_pair.first, canvas->output_pair.second);
}

WaylandCanvas::WaylandCanvas()
    : display(wl_display_connect(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Failed to connect to wayland display.");
    }
    logger = spdlog::get("wayland");
    flags = Flags::instance();
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);
    config = WaylandConfig::get();
    event_handler = std::thread([this] { handle_events(); });

#ifdef ENABLE_OPENGL
    if (flags->use_opengl) {
        try {
            egl = std::make_unique<EGLUtil<struct wl_display, struct wl_egl_window>>(EGL_PLATFORM_WAYLAND_EXT, display);
        } catch (const std::runtime_error &err) {
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
    stop_flag.store(true);
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
    const int waitms = 100;
    const auto wl_fd = wl_display_get_fd(display);

    while (!stop_flag.load()) {
        // prepare to read wayland events
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }
        wl_display_flush(display);

        // poll events
        try {
            const auto in_event = os::wait_for_data_on_fd(wl_fd, waitms);
            if (in_event) {
                wl_display_read_events(display);
                wl_display_dispatch_pending(display);
            } else {
                wl_display_cancel_read(display);
            }
        } catch (const std::system_error &err) {
            wl_display_cancel_read(display);
        }
    }
}

void WaylandCanvas::remove_image(const std::string &identifier)
{
    windows.erase(identifier);
    wl_display_flush(display);
}
