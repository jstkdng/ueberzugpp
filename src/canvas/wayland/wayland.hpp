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

#ifndef WAYLAND_CANVAS_H
#define WAYLAND_CANVAS_H

#include "canvas.hpp"
#include "config.hpp"
#include "flags.hpp"
#include "wayland-xdg-shell-client-protocol.h"
#include "window/waylandwindow.hpp"

#include <memory>
#include <thread>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <wayland-client.h>

#ifdef ENABLE_OPENGL
#  include "util/egl.hpp"
#  include <wayland-egl.h>
#endif

class WaylandCanvas : public Canvas
{
  public:
    explicit WaylandCanvas();
    ~WaylandCanvas() override;

    static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                                       uint32_t version);
    static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);

    static void output_scale(void *data, struct wl_output *output, int32_t scale);
    static void output_name(void *data, struct wl_output *output, const char *name);
    static void output_done(void *data, struct wl_output *output);

    void add_image(const std::string &identifier, std::unique_ptr<Image> new_image) override;
    void remove_image(const std::string &identifier) override;
    void show() override;
    void hide() override;

    struct wl_compositor *compositor = nullptr;
    struct wl_shm *wl_shm = nullptr;
    struct xdg_wm_base *xdg_base = nullptr;

    std::pair<std::string, int32_t> output_pair;
    std::unordered_map<std::string, int32_t> output_info;

  private:
    struct wl_display *display = nullptr;
    struct wl_registry *registry = nullptr;
    std::thread event_handler;

    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<WaylandConfig> config;
    std::shared_ptr<Flags> flags;
    std::unordered_map<std::string, std::shared_ptr<WaylandWindow>> windows;

#ifdef ENABLE_OPENGL
    std::unique_ptr<EGLUtil<struct wl_display, struct wl_egl_window>> egl;
    bool egl_available = true;
#endif

    struct XdgStructAgg xdg_agg;

    void handle_events();
};

#endif
