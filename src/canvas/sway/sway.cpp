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

#include "sway.hpp"

#include <iostream>
#include <array>
#include <string_view>
#include <fmt/format.h>
#include <cstring>
#include <poll.h>

constexpr struct wl_registry_listener registry_listener = {
    .global = SwayCanvas::registry_handle_global,
    .global_remove = SwayCanvas::registry_handle_global_remove
};

constexpr struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = SwayCanvas::xdg_wm_base_ping
};

constexpr struct xdg_surface_listener xdg_surface_listener = {
    .configure = SwayCanvas::xdg_surface_configure,
};

void SwayCanvas::registry_handle_global(void *data, wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    std::string_view interface_str { interface };
    auto *canvas = reinterpret_cast<SwayCanvas*>(data);
    if (interface_str == wl_compositor_interface.name) {
        canvas->compositor = reinterpret_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, version)
        );
    } else if (interface_str == wl_shm_interface.name) {
        canvas->wl_shm = reinterpret_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, version)
        );
    } else if (interface_str == xdg_wm_base_interface.name) {
        canvas->xdg_base = reinterpret_cast<struct xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, version)
        );
        xdg_wm_base_add_listener(canvas->xdg_base, &xdg_wm_base_listener, canvas);
    }
}

void SwayCanvas::registry_handle_global_remove(
        [[maybe_unused]] void *data,
        [[maybe_unused]] wl_registry *registry,
        [[maybe_unused]] uint32_t name)
{}

void SwayCanvas::xdg_wm_base_ping([[maybe_unused]] void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void SwayCanvas::xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    auto *canvas = reinterpret_cast<SwayCanvas*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    std::memcpy(canvas->shm->get_data(), canvas->image->data(), canvas->image->size());
    wl_surface_attach(canvas->surface, canvas->shm->buffer, 0, 0);
    wl_surface_commit(canvas->surface);
}

SwayCanvas::SwayCanvas():
display(wl_display_connect(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Failed to connect to wayland display.");
    }
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);

    const auto cur_window = socket.current_window();
    const auto cur_workspace = socket.current_workspace();
    const auto& win_rect = cur_window["window_rect"];
    const auto& global_rect = cur_window["rect"];
    const auto& wrks_rect = cur_workspace["rect"];
    sway_x = static_cast<int>(win_rect["x"]) +
        static_cast<int>(global_rect["x"]) -
        static_cast<int>(wrks_rect["x"]);
    sway_y = static_cast<int>(win_rect["y"]) +
        static_cast<int>(global_rect["y"]) -
        static_cast<int>(wrks_rect["y"]);
}

SwayCanvas::~SwayCanvas()
{
    shm.reset();
    if (wl_shm != nullptr) {
        wl_shm_destroy(wl_shm);
    }
    if (compositor != nullptr) {
        wl_compositor_destroy(compositor);
    }
    if (registry != nullptr) {
        wl_registry_destroy(registry);
    }
    if (surface != nullptr) {
        wl_surface_destroy(surface);
    }
    wl_display_disconnect(display);
}

void SwayCanvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    image = std::move(new_image);
    x = sway_x + dimensions.xpixels();
    y = sway_y + dimensions.ypixels();
    width = image->width();
    height = image->height();

    // TODO: change appid
    socket.ipc_command("no_focus [app_id=ueberzugpp]");
    socket.ipc_command("ueberzugpp", "floating enable");
    socket.ipc_command("ueberzugpp", fmt::format("move absolute position {} {}", x, y));

    event_handler = std::thread([&] {
        handle_events();
    });
}

void SwayCanvas::handle_events()
{
    const int waitms = 100;
    struct pollfd fds;
    fds.fd = wl_display_get_fd(display);
    fds.events = POLLIN;

    while (!stop_flag.load()) {
        // prepare to read wayland events
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }
        wl_display_flush(display);

        // poll events
        if (poll(&fds, 1, waitms) <= 0) {
            wl_display_cancel_read(display);
            continue;
        }

        // read and handle wayland events
        if ((fds.revents & POLLIN) != 0) {
            wl_display_read_events(display);
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }
    }
}

void SwayCanvas::draw()
{
    shm = std::make_unique<SwayShm>(width, height, wl_shm);
    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_set_app_id(xdg_toplevel, "ueberzugpp");
    xdg_toplevel_set_title(xdg_toplevel, "ueberzugpp: image window");
    wl_surface_commit(surface);
}

void SwayCanvas::clear()
{
    if (surface == nullptr) {
        return;
    }
    stop_flag.store(true);
    //wl_surface_attach(surface, shm->buffer, 0, 0);
    wl_surface_commit(surface);
    if (event_handler.joinable()) {
        event_handler.join();
    }
    stop_flag.store(false);
    if (xdg_toplevel != nullptr) {
        xdg_toplevel_destroy(xdg_toplevel);
    }
    if (xdg_surface != nullptr) {
        xdg_surface_destroy(xdg_surface);
    }
    if (surface != nullptr) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }
    wl_display_flush(display);
    shm.reset();
}
