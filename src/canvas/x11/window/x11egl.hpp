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

#ifndef X11_EGL_WINDOW_H
#define X11_EGL_WINDOW_H

#include "window.hpp"
#include "util/egl.hpp"

#include <xcb/xcb.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>

class Image;

class X11EGLWindow : public Window
{
public:
    X11EGLWindow(xcb_connection_t* connection, xcb_screen_t* screen,
            xcb_window_t windowid, xcb_window_t parentid, const EGLUtil<xcb_connection_t, xcb_window_t>* egl,
            std::shared_ptr<Image> new_image);
    ~X11EGLWindow() override;

    void draw() override;
    void generate_frame() override;
    void show() override;
    void hide() override;

private:
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t windowid;
    xcb_window_t parentid;
    std::shared_ptr<Image> image;
    const EGLUtil<xcb_connection_t, xcb_window_t>* egl;

    GLuint texture;
    GLuint fbo;
    EGLContext egl_context;
    EGLSurface egl_surface;

    std::mutex egl_mutex;
    std::shared_ptr<spdlog::logger> logger;

    bool visible = false;

    void send_expose_event();
    void create();
    void change_title();
    void opengl_setup();
};

#endif
