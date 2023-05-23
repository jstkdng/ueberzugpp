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

#ifndef __SWAY_CANVAS__
#define __SWAY_CANVAS__

#include "canvas.hpp"
#include "dimensions.hpp"
#include "terminal.hpp"
#include "shm.hpp"
#include "wayland-xdg-shell-client-protocol.h"

#include <wayland-client.h>
#include <memory>
#include <atomic>
#include <thread>

class SwayCanvas : public Canvas
{
public:
    explicit SwayCanvas();
    ~SwayCanvas() override;

    static void registry_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version);
    static void registry_handle_global_remove(void *data, struct wl_registry *registry,
        uint32_t name);
    static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
    static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);

    void init(const Dimensions& dimensions, std::unique_ptr<Image> new_image) override;
    void draw() override;
    void clear() override;

    struct wl_compositor* compositor = nullptr;
    struct wl_output* output = nullptr;
    struct wl_surface* surface = nullptr;
    struct wl_shm* wl_shm = nullptr;
    struct xdg_wm_base* xdg_base = nullptr;
    struct xdg_surface* xdg_surface = nullptr;
    struct xdg_toplevel* xdg_toplevel = nullptr;
    std::unique_ptr<SwayShm> shm;
    std::unique_ptr<Image> image;

    int width;
    int height;

private:
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    std::atomic<bool> stop_flag {false};
    std::thread event_handler;

    void handle_events();

    int x;
    int y;
};

#endif
