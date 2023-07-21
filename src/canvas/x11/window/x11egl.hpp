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

#include <memory>

class Image;

class X11EGLWindow : public Window
{
public:
    X11EGLWindow(xcb_connection_t* connection, xcb_screen_t* screen,
            xcb_window_t windowid, xcb_window_t parentid, EGLDisplay egl_display,
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
    xcb_gcontext_t gc;
    EGLDisplay egl_display;
    EGLSurface egl_surface;
    EGLUtil egl_util;

    GLuint texture;
    GLuint buffer;
    GLuint vbo;
    GLuint vao;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint shader_program;


    std::shared_ptr<Image> image;

    bool visible = false;

    void send_expose_event();
    void create();
    void change_title();
};

#endif
