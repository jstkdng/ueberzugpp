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
#include <iostream>

constexpr int id_len = 10;

constexpr struct xdg_surface_listener xdg_surface_listener = {
    .configure = WaylandEglWindow::xdg_surface_configure,
};

constexpr struct wl_callback_listener frame_listener = {
    .done = WaylandEglWindow::wl_surface_frame_done
};

struct XdgStruct
{
    std::weak_ptr<WaylandEglWindow> ptr;
};
struct XdgStructAgg
{
    std::vector<std::unique_ptr<XdgStruct>> ptrs;
};
const auto xdgs = std::make_unique<struct XdgStructAgg>();

WaylandEglWindow::WaylandEglWindow(struct wl_compositor *compositor, struct xdg_wm_base *xdg_base,
        EGLDisplay egl_display, std::unique_ptr<Image> new_image, std::shared_ptr<WaylandConfig> new_config):
compositor(compositor),
xdg_base(xdg_base),
surface(wl_compositor_create_surface(compositor)),
xdg_surface(xdg_wm_base_get_xdg_surface(xdg_base, surface)),
xdg_toplevel(xdg_surface_get_toplevel(xdg_surface)),
image(std::move(new_image)),
config(std::move(new_config)),
egl_display(egl_display),
egl_util(egl_display),
egl_window(wl_egl_window_create(surface, image->width(), image->height())),
appid(fmt::format("ueberzugpp_{}", util::generate_random_string(id_len)))
{
    config->initial_setup(appid);
    xdg_setup();
    /*
    egl_window = wl_egl_window_create(surface, image->width(), image->height());
    egl_surface = eglCreatePlatformWindowSurface(egl_display, egl_util.config, egl_window, nullptr);

    if (egl_surface == EGL_NO_SURFACE) {
        std::cout << "Could not create surface" << std::endl;
    }

    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_base, surface);
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);


    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
#ifdef DEBUG
    glDebugMessageCallback(EGLUtil::debug_callback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);*/
}

WaylandEglWindow::~WaylandEglWindow()
{
    delete_xdg_structs();
    delete_wayland_structs();

    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &fbo);
    eglDestroySurface(egl_display, egl_surface);
}

void WaylandEglWindow::finish_init()
{
    auto xdg = std::make_unique<XdgStruct>();
    xdg->ptr = shared_from_this();
    this_ptr = xdg.get();
    xdgs->ptrs.push_back(std::move(xdg));
    setup_listeners();
    visible = true;
}

void WaylandEglWindow::setup_listeners()
{
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, this_ptr);
    wl_surface_commit(surface);

    if (image->is_animated()) {
        callback = wl_surface_frame(surface);
        wl_callback_add_listener(callback, &frame_listener, this_ptr);
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
    if (surface != nullptr) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }
    if (egl_window != nullptr) {
        wl_egl_window_destroy(egl_window);
        egl_window = nullptr;
    }
}

void WaylandEglWindow::draw()
{
    const std::scoped_lock lock {egl_mutex};
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    glBlitFramebuffer(0, 0, image->width(), image->height(), 0, 0, image->width(), image->height(),
                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

    eglSwapBuffers(egl_display, egl_surface);
    wl_surface_commit(surface);
    move_window(); 
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
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
    const std::scoped_lock lock {egl_mutex};
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    EGLUtil::get_texture_from_image(*image, texture);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, texture, 0);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

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
}

void WaylandEglWindow::xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
    const auto *tmp = reinterpret_cast<struct XdgStruct*>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    window->draw();
}

void WaylandEglWindow::wl_surface_frame_done(void *data, struct wl_callback *callback, [[maybe_unused]] uint32_t time)
{
    wl_callback_destroy(callback);
    const auto *tmp = reinterpret_cast<struct XdgStruct*>(data);
    const auto window = tmp->ptr.lock();
    if (!window) {
        return;
    }
    const std::scoped_lock lock {window->draw_mutex};
    if (!window->visible) {
        return;
    }
    window->generate_frame();
}

