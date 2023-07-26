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

#ifndef WAYLAND_SHM_WINDOW_H
#define WAYLAND_SHM_WINDOW_H

#include "waylandwindow.hpp"
#include "wayland-xdg-shell-client-protocol.h"

#include <wayland-client.h>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

class Image;
class WaylandConfig;
class WaylandShm;

class WaylandShmWindow :
    public WaylandWindow
{
public:
    WaylandShmWindow(struct wl_compositor *compositor, struct wl_shm *wl_shm,
            struct xdg_wm_base *xdg_base, std::unique_ptr<Image> new_image,
            std::shared_ptr<WaylandConfig> new_config, struct XdgStructAgg& xdg_agg);
    ~WaylandShmWindow() override;
    static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
    static void wl_surface_frame_done(void *data, struct wl_callback *callback, uint32_t time);

    void draw() override;
    void generate_frame() override;
    void show() override;
    void hide() override;

    void finish_init() override;

    std::mutex draw_mutex;
    std::atomic<bool> visible {false};

private:
    struct wl_compositor *compositor;
    struct xdg_wm_base *xdg_base;

    struct wl_surface *surface = nullptr;
    struct xdg_surface *xdg_surface = nullptr;
    struct xdg_toplevel *xdg_toplevel = nullptr;
    struct wl_callback *callback;
    std::unique_ptr<Image> image;
    std::unique_ptr<WaylandShm> shm;
    std::string appid;
    std::shared_ptr<WaylandConfig> config;

    XdgStructAgg& xdg_agg;
    void* this_ptr;

    void move_window();
    void xdg_setup();

    void setup_listeners();
    void delete_wayland_structs();
};

#endif
