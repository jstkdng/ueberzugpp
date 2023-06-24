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
#include "os.hpp"
#include "util.hpp"
#include "image.hpp"

#include <iostream>
#include <array>
#include <string_view>
#include <cstring>

constexpr struct wl_registry_listener registry_listener = {
    .global = WaylandCanvas::registry_handle_global,
    .global_remove = WaylandCanvas::registry_handle_global_remove
};

constexpr struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = WaylandCanvas::xdg_wm_base_ping
};

constexpr struct xdg_surface_listener xdg_surface_listener = {
    .configure = WaylandCanvas::xdg_surface_configure,
};

constexpr struct wl_callback_listener frame_listener = {
    .done = WaylandCanvas::wl_surface_frame_done
};

void WaylandCanvas::registry_handle_global(void *data, wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    std::string_view interface_str { interface };
    auto *canvas = reinterpret_cast<WaylandCanvas*>(data);
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

void WaylandCanvas::registry_handle_global_remove(
        [[maybe_unused]] void *data,
        [[maybe_unused]] wl_registry *registry,
        [[maybe_unused]] uint32_t name)
{}

void WaylandCanvas::xdg_wm_base_ping([[maybe_unused]] void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandCanvas::xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    auto *canvas = reinterpret_cast<WaylandCanvas*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    std::unique_lock lock {canvas->draw_mutex, std::defer_lock};
    if (canvas->image->is_animated()) {
        lock.lock();
        if (!canvas->can_draw.load()) {
            return;
        }
    }

    std::memcpy(canvas->shm->get_data(), canvas->image->data(), canvas->image->size());
    wl_surface_attach(canvas->surface, canvas->shm->buffer, 0, 0);
    wl_surface_commit(canvas->surface);
}

void WaylandCanvas::wl_surface_frame_done(void *data, struct wl_callback *callback, [[maybe_unused]] uint32_t time)
{
    auto *canvas = reinterpret_cast<WaylandCanvas*>(data);
    wl_callback_destroy(callback);
    std::unique_lock lock {canvas->draw_mutex};
    if (!canvas->can_draw.load()) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(canvas->image->frame_delay()));

    // request another callback
    callback = wl_surface_frame(canvas->surface);
    wl_callback_add_listener(callback, &frame_listener, canvas);

    canvas->image->next_frame();
    std::memcpy(canvas->shm->get_data(), canvas->image->data(), canvas->image->size());
    wl_surface_attach(canvas->surface, canvas->shm->buffer, 0, 0);
    wl_surface_damage_buffer(canvas->surface, 0, 0, canvas->width, canvas->height);
    wl_surface_commit(canvas->surface);
}

WaylandCanvas::WaylandCanvas():
display(wl_display_connect(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Failed to connect to wayland display.");
    }
    logger = spdlog::get("wayland");
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);
    config = WaylandConfig::get();
    event_handler = std::thread([&] {
        handle_events();
    });

    const int idlength = 10;
    appid = fmt::format("ueberzugpp_{}", util::generate_random_string(idlength));

    config->initial_setup(appid);
    logger->info("Canvas created");
}

void WaylandCanvas::show()
{
    if (visible) {
        return;
    }
    visible = true;
    draw();
}

void WaylandCanvas::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    remove_image("");
}

WaylandCanvas::~WaylandCanvas()
{
    can_draw.store(false);
    std::unique_lock lock {draw_mutex};
    stop_flag.store(true);
    if (event_handler.joinable()) {
        event_handler.join();
    }
    stop_flag.store(false);
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

void WaylandCanvas::add_image(const std::string& identifier, std::unique_ptr<Image> new_image)
{
    remove_image(identifier);
    image = std::move(new_image);
    const auto dims = image->dimensions();
    wayland_x = dims.xpixels() + dims.padding_horizontal;
    wayland_y = dims.ypixels() + dims.padding_vertical;
    width = image->width();
    height = image->height();
    draw();
}

void WaylandCanvas::move_window()
{
    const auto cur_window = config->get_window_info();
    const int xcoord = cur_window.x + wayland_x;
    const int ycoord = cur_window.y + wayland_y;
    config->move_window(appid, xcoord, ycoord);
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
        } catch (const std::system_error& err) {
            wl_display_cancel_read(display);
        }
    }
}

void WaylandCanvas::draw()
{
    shm = std::make_unique<WaylandShm>(width, height, wl_shm);
    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_set_app_id(xdg_toplevel, appid.c_str());
    xdg_toplevel_set_title(xdg_toplevel, appid.c_str());
    wl_surface_commit(surface);
    move_window();

    if (image->is_animated()) {
        callback =  wl_surface_frame(surface);
        wl_callback_add_listener(callback, &frame_listener, this);
        can_draw.store(true);
    }
    visible = true;
}

void WaylandCanvas::remove_image([[maybe_unused]] const std::string& identifier)
{
    if (surface == nullptr) {
        return;
    }
    can_draw.store(false);
    std::unique_lock lock {draw_mutex};
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
