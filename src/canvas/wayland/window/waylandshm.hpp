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

#include "../wayland.hpp"
#include "image.hpp"
#include "shm.hpp"
#include "wayland-xdg-shell-client-protocol.h"
#include "waylandwindow.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <wayland-client.h>

class WaylandShmWindow : public WaylandWindow
{
  public:
    WaylandShmWindow(WaylandCanvas *canvas, std::unique_ptr<Image> new_image, struct XdgStructAgg *xdg_agg,
                     WaylandConfig *config);
    ~WaylandShmWindow() override;
    static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
    static void wl_surface_frame_done(void *data, struct wl_callback *callback, uint32_t time);

    void draw() override {}
    void wl_draw(int32_t scale_factor) override;
    void generate_frame() override;
    void show() override;
    void hide() override;

    void finish_init() override;

    std::mutex draw_mutex;
    std::atomic<bool> visible{false};
    std::unique_ptr<WaylandShm> shm;
    int32_t output_scale;

  private:
    WaylandConfig *config;

    struct xdg_wm_base *xdg_base = nullptr;
    struct wl_surface *surface = nullptr;
    struct xdg_surface *xdg_surface = nullptr;
    struct xdg_toplevel *xdg_toplevel = nullptr;
    struct wl_callback *callback;

    std::unique_ptr<Image> image;
    std::string appid;

    struct XdgStructAgg *xdg_agg;
    void *this_ptr;

    void move_window();
    void xdg_setup();

    void setup_listeners();
    void delete_wayland_structs();
    void delete_xdg_structs();
};

#endif
