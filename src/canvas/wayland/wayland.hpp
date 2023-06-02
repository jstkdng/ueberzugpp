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

#ifndef __WAYLAND_CANVAS__
#define __WAYLAND_CANVAS__

#include "canvas.hpp"
#include "dimensions.hpp"
#include "terminal.hpp"
#include "shm.hpp"
#include "config.hpp"
#include "wayland-xdg-shell-client-protocol.h"

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

#include <fmt/format.h>
#include <wayland-client.h>

class WaylandCanvas : public Canvas
{
public:
    explicit WaylandCanvas();
    ~WaylandCanvas() override;

    static void registry_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version);
    static void registry_handle_global_remove(void *data, struct wl_registry *registry,
        uint32_t name);
    static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
    static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
    static void wl_surface_frame_done(void *data, struct wl_callback *callback, uint32_t time);

    void init(const Dimensions& dimensions, std::unique_ptr<Image> new_image) override;
    void draw() override;
    void clear() override;

    void show() override;
    void hide() override;
    void toggle() override;

    struct wl_compositor* compositor = nullptr;
    struct wl_surface* surface = nullptr;
    struct wl_shm* wl_shm = nullptr;
    struct wl_callback* callback = nullptr;
    struct xdg_wm_base* xdg_base = nullptr;
    struct xdg_surface* xdg_surface = nullptr;
    struct xdg_toplevel* xdg_toplevel = nullptr;
    std::unique_ptr<WaylandShm> shm;
    std::unique_ptr<Image> image;
    std::shared_ptr<spdlog::logger> logger;
    std::mutex draw_mutex;
    std::atomic<bool> can_draw {false};

    int width;
    int height;

private:
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    std::string appid;
    std::atomic<bool> stop_flag {false};
    std::thread event_handler;

    std::unique_ptr<WaylandConfig> config;

    void handle_events();

    bool visible = false;
};

#endif
