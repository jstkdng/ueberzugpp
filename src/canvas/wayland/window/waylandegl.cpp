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

#include "waylandegl.hpp"
#include "util.hpp"
#include "image.hpp"
#include "dimensions.hpp"
#include "../config.hpp"

#include <fmt/format.h>
#include <thread>

constexpr int id_len = 10;

constexpr struct xdg_surface_listener xdg_surface_listener_egl = {
    .configure = WaylandEglWindow::xdg_surface_configure,
};

constexpr struct wl_callback_listener frame_listener_egl = {
    .done = WaylandEglWindow::wl_surface_frame_done
};

WaylandEglWindow::WaylandEglWindow(struct wl_compositor *compositor, struct xdg_wm_base *xdg_base,
        const EGLUtil<struct wl_display, struct wl_egl_window>* egl, std::unique_ptr<Image> new_image,
        std::shared_ptr<WaylandConfig> new_config, struct XdgStructAgg* xdg_agg):
compositor(compositor),
xdg_base(xdg_base),
surface(wl_compositor_create_surface(compositor)),
xdg_surface(xdg_wm_base_get_xdg_surface(xdg_base, surface)),
xdg_toplevel(xdg_surface_get_toplevel(xdg_surface)),
image(std::move(new_image)),
config(std::move(new_config)),
egl_window(wl_egl_window_create(surface, image->width(), image->height())),
egl(egl),
appid(fmt::format("ueberzugpp_{}", util::generate_random_string(id_len))),
xdg_agg(xdg_agg)
{
    config->initial_setup(appid);
    opengl_setup();
    xdg_setup();
}

WaylandEglWindow::~WaylandEglWindow()
{
    opengl_cleanup();
    delete_xdg_structs();
    delete_wayland_structs();
}

void WaylandEglWindow::opengl_cleanup()
{
    egl->run_contained(egl_context, egl_surface, [this] {
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &fbo);
    });
    eglDestroySurface(egl->display, egl_surface);
    eglDestroyContext(egl->display, egl_context);
}

void WaylandEglWindow::finish_init()
{
    auto xdg = std::make_unique<XdgStruct>();
    xdg->ptr = weak_from_this();
    this_ptr = xdg.get();
    xdg_agg->ptrs.push_back(std::move(xdg));
    setup_listeners();
    visible = true;
}

void WaylandEglWindow::opengl_setup()
{
    egl_surface = egl->create_surface(egl_window);
    if (egl_surface == EGL_NO_SURFACE) {
        throw std::runtime_error("");
    }

    egl_context = egl->create_context(egl_surface);
    if (egl_context == EGL_NO_CONTEXT) {
        throw std::runtime_error("");
    }

    egl->run_contained(egl_surface, egl_context, [this] {
        eglSwapInterval(egl->display, 0);
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &texture);
    });
}

void WaylandEglWindow::setup_listeners()
{
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener_egl, this_ptr);
    wl_surface_commit(surface);

    if (image->is_animated()) {
        callback = wl_surface_frame(surface);
        wl_callback_add_listener(callback, &frame_listener_egl, this_ptr);
    }
}

void WaylandEglWindow::xdg_setup()
{
    xdg_toplevel_set_app_id(xdg_toplevel, appid.c_str());
    xdg_toplevel_set_title(xdg_toplevel, appid.c_str());
}

void WaylandEglWindow::delete_xdg_structs()
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

void WaylandEglWindow::delete_wayland_structs()
{
    if (egl_window != nullptr) {
        wl_egl_window_destroy(egl_window);
        egl_window = nullptr;
    }
    if (surface != nullptr) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }
}

void WaylandEglWindow::draw()
{
    load_framebuffer();

    wl_surface_commit(surface);
    move_window();
}

void WaylandEglWindow::load_framebuffer()
{
    std::scoped_lock lock {egl_mutex};
    egl->run_contained(egl_surface, egl_context, [this] {
        egl->get_texture_from_image(*image, texture);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, texture, 0);
        glBlitFramebuffer(0, 0, image->width(), image->height(), 0, 0, image->width(), image->height(),
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

        eglSwapBuffers(egl->display, egl_surface);
    });
}

void WaylandEglWindow::move_window()
{
    const auto dims = image->dimensions();
    const auto cur_window = config->get_window_info();
    const int wayland_x = dims.xpixels() + dims.padding_horizontal;
    const int wayland_y = dims.ypixels() + dims.padding_vertical;
    const int xcoord = cur_window.x + wayland_x;
    const int ycoord = cur_window.y + wayland_y;
    config->move_window(appid, xcoord, ycoord);
}

void WaylandEglWindow::generate_frame()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
    callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener_egl, this_ptr);

    image->next_frame();
    load_framebuffer();

    wl_surface_commit(surface);
}

void WaylandEglWindow::show()
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

void WaylandEglWindow::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    const std::scoped_lock lock {draw_mutex};
    delete_xdg_structs();
    wl_surface_attach(surface, nullptr, 0, 0);
    wl_surface_commit(surface);
}

void WaylandEglWindow::xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
    const auto *tmp = reinterpret_cast<struct XdgStruct*>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    auto* egl_window = dynamic_cast<WaylandEglWindow*>(window.get());
    egl_window->draw();
}

void WaylandEglWindow::wl_surface_frame_done(void *data, struct wl_callback *callback, [[maybe_unused]] uint32_t time)
{
    wl_callback_destroy(callback);
    const auto *tmp = reinterpret_cast<struct XdgStruct*>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    auto* egl_window = dynamic_cast<WaylandEglWindow*>(window.get());
    const std::scoped_lock lock {egl_window->draw_mutex};
    if (!egl_window->visible) {
        return;
    }
    egl_window->generate_frame();
}

