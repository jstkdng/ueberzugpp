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

#ifndef __WAYLAND_SHM_WINDOW__
#define __WAYLAND_SHM_WINDOW__

#include "window.hpp"
#include "../shm.hpp"
#include "util/ptr.hpp"
#include "wayland-xdg-shell-client-protocol.h"

#include <wayland-client.h>
#include <mutex>
#include <string>

class Image;
class WaylandConfig;

class WaylandShmWindow : public Window, public std::enable_shared_from_this<WaylandShmWindow>
{
public:
    WaylandShmWindow(struct wl_compositor *compositor, struct wl_shm *wl_shm,
            struct xdg_wm_base *xdg_base, std::unique_ptr<Image> new_image,
            std::shared_ptr<WaylandConfig> new_config);
    ~WaylandShmWindow() override;
    static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
    static void wl_surface_frame_done(void *data, struct wl_callback *callback, uint32_t time);

    void draw() override;
    void generate_frame() override;
    void finish_init();

private:
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_callback *callback;
    std::unique_ptr<Image> image;
    WaylandShm shm;
    std::string appid;
    std::shared_ptr<WaylandConfig> config;
    bool visible = false;

    void move_window();
};

#endif
