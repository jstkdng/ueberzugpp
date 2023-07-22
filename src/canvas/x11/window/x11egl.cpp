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

#include "image.hpp"
#include "dimensions.hpp"
#include "x11egl.hpp"
#include "util.hpp"

#include <iostream>
#include <array>

#include <gsl/gsl>

X11EGLWindow::X11EGLWindow(xcb_connection_t* connection, xcb_screen_t* screen,
            xcb_window_t windowid, xcb_window_t parentid, EGLDisplay egl_display,
            std::shared_ptr<Image> new_image):
connection(connection),
screen(screen),
windowid(windowid),
parentid(parentid),
gc(xcb_generate_id(connection)),
egl_display(egl_display),
egl_util(egl_display),
image(std::move(new_image))
{
    create();
    egl_surface = eglCreatePlatformWindowSurface(egl_display, egl_util.config,
            &windowid, nullptr);
    if (egl_surface == EGL_NO_SURFACE) {
        std::cout << "Could not create surface" << std::endl;
    }

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
#ifdef DEBUG
    glDebugMessageCallback(EGLUtil::debug_callback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

X11EGLWindow::~X11EGLWindow()
{
    xcb_destroy_window(connection, windowid);
    xcb_flush(connection);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &fbo);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl_display, egl_surface);
}

void X11EGLWindow::create()
{
    const uint32_t value_mask =  XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    struct xcb_create_window_value_list_t value_list;
    value_list.background_pixel = screen->black_pixel;
    value_list.border_pixel = screen->black_pixel;
    value_list.event_mask = XCB_EVENT_MASK_EXPOSURE;
    value_list.colormap = screen->default_colormap;

    const auto dimensions = image->dimensions();
    const auto xcoord = gsl::narrow_cast<int16_t>(dimensions.xpixels() + dimensions.padding_horizontal);
    const auto ycoord = gsl::narrow_cast<int16_t>(dimensions.ypixels() + dimensions.padding_vertical);
    xcb_create_window_aux(connection,
            screen->root_depth,
            windowid,
            parentid,
            xcoord, ycoord,
            image->width(), image->height(),
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            value_mask,
            &value_list);
}

void X11EGLWindow::draw()
{
    const std::scoped_lock lock {egl_mutex};
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    glBlitFramebuffer(0, 0, image->width(), image->height(), 0, 0, image->width(), image->height(),
                  GL_COLOR_BUFFER_BIT, GL_NEAREST);

    eglSwapBuffers(egl_display, egl_surface);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void X11EGLWindow::generate_frame()
{
    const std::scoped_lock lock {egl_mutex};
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);

    EGLUtil::get_texture_from_image(*image, texture);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, texture, 0);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    send_expose_event();
}

void X11EGLWindow::show()
{
    if (visible) {
        return;
    }
    visible = true;
    xcb_map_window(connection, windowid);
    xcb_flush(connection);
}

void X11EGLWindow::hide()
{
    if (!visible) {
        return;
    }
    visible = false;
    xcb_unmap_window(connection, windowid);
    xcb_flush(connection);
}

void X11EGLWindow::send_expose_event()
{
    const int event_size = 32;
    std::array<char, event_size> buffer;
    auto *event = reinterpret_cast<xcb_expose_event_t*>(buffer.data());
    event->response_type = XCB_EXPOSE;
    event->window = windowid;
    xcb_send_event(connection, 0, windowid,
            XCB_EVENT_MASK_EXPOSURE, reinterpret_cast<char*>(event));
    xcb_flush(connection);
}
