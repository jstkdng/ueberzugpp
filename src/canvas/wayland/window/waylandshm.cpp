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

#include "waylandshm.hpp"
#include "../config.hpp"
#include "dimensions.hpp"
#include "image.hpp"
#include "shm.hpp"
#include "util.hpp"

#include <fmt/format.h>
#include <thread>

constexpr int id_len = 10;

constexpr struct xdg_surface_listener xdg_surface_listener = {
    .configure = WaylandShmWindow::xdg_surface_configure,
};

constexpr struct wl_callback_listener frame_listener = {.done = WaylandShmWindow::wl_surface_frame_done};

WaylandShmWindow::WaylandShmWindow(struct wl_compositor *compositor, struct wl_shm *wl_shm,
                                   struct xdg_wm_base *xdg_base, std::unique_ptr<Image> new_image,
                                   std::shared_ptr<WaylandConfig> new_config, struct XdgStructAgg *xdg_agg,
                                   int32_t output_scale_factor)
    : compositor(compositor),
      xdg_base(xdg_base),
      surface(wl_compositor_create_surface(compositor)),
      xdg_surface(xdg_wm_base_get_xdg_surface(xdg_base, surface)),
      xdg_toplevel(xdg_surface_get_toplevel(xdg_surface)),
      scale_factor(output_scale_factor),
      image(std::move(new_image)),
      shm(std::make_unique<WaylandShm>(image->width(), image->height(), scale_factor, wl_shm)),
      appid(fmt::format("ueberzugpp_{}", util::generate_random_string(id_len))),
      config(std::move(new_config)),
      xdg_agg(xdg_agg)
{
    config->initial_setup(appid);
    xdg_setup();
}

void WaylandShmWindow::finish_init()
{
    auto xdg = std::make_unique<XdgStruct>();
    xdg->ptr = weak_from_this();
    this_ptr = xdg.get();
    xdg_agg->ptrs.push_back(std::move(xdg));
    setup_listeners();
    visible = true;
}

void WaylandShmWindow::setup_listeners()
{
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this_ptr);
    wl_surface_commit(surface);

    if (image->is_animated()) {
        callback = wl_surface_frame(surface);
        wl_callback_add_listener(callback, &frame_listener, this_ptr);
    }
}

void WaylandShmWindow::xdg_setup()
{
    xdg_toplevel_set_app_id(xdg_toplevel, appid.c_str());
    xdg_toplevel_set_title(xdg_toplevel, appid.c_str());
}

WaylandShmWindow::~WaylandShmWindow()
{
    delete_xdg_structs();
    delete_wayland_structs();
}

void WaylandShmWindow::wl_draw(int32_t scale_factor)
{
    std::memcpy(shm->get_data(), image->data(), image->size());
    wl_surface_attach(surface, shm->buffer, 0, 0);
    wl_surface_set_buffer_scale(surface, scale_factor);
    wl_surface_commit(surface);
    move_window();
}

void WaylandShmWindow::show()
{
    if (visible) {
        return;
    }
    visible = true;
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_base, surface);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_setup();
    setup_listeners();
}

void WaylandShmWindow::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    const std::scoped_lock lock{draw_mutex};
    delete_xdg_structs();
    wl_surface_attach(surface, nullptr, 0, 0);
    wl_surface_commit(surface);
}

void WaylandShmWindow::delete_xdg_structs()
{
    if (xdg_toplevel != nullptr) {
        xdg_toplevel_destroy(xdg_toplevel);
        xdg_toplevel = nullptr;
    }
    if (xdg_surface != nullptr) {
        xdg_surface_destroy(xdg_surface);
        xdg_surface = nullptr;
    }
}

void WaylandShmWindow::delete_wayland_structs()
{
    if (surface != nullptr) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }
}

void WaylandShmWindow::move_window()
{
    const auto dims = image->dimensions();
    const auto cur_window = config->get_window_info();
    const int wayland_x = dims.xpixels() + dims.padding_horizontal;
    const int wayland_y = dims.ypixels() + dims.padding_vertical;
    const int xcoord = cur_window.x + wayland_x;
    const int ycoord = cur_window.y + wayland_y;
    config->move_window(appid, xcoord, ycoord);
}

void WaylandShmWindow::generate_frame()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
    callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener, this_ptr);

    image->next_frame();
    std::memcpy(shm->get_data(), image->data(), image->size());
    wl_surface_attach(surface, shm->buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, image->width(), image->height());
    wl_surface_commit(surface);
}

void WaylandShmWindow::xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
    const auto *tmp = reinterpret_cast<struct XdgStruct *>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    auto *shm_window = dynamic_cast<WaylandShmWindow *>(window.get());
    shm_window->wl_draw(shm_window->scale_factor);
}

void WaylandShmWindow::wl_surface_frame_done(void *data, struct wl_callback *callback, [[maybe_unused]] uint32_t time)
{
    wl_callback_destroy(callback);
    const auto *tmp = reinterpret_cast<struct XdgStruct *>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    auto *shm_window = dynamic_cast<WaylandShmWindow *>(window.get());
    const std::scoped_lock lock{shm_window->draw_mutex};
    if (!shm_window->visible) {
        return;
    }
    shm_window->generate_frame();
}
