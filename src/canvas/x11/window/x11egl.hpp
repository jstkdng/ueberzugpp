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

#ifndef __X11EGLWINDOW__
#define __X11EGLWINDOW__

#include "window.hpp"

#include <EGL/egl.h>
#include <xcb/xcb.h>

#include <memory>

class Image;

class X11EGLWindow : public Window
{
public:
    X11EGLWindow(xcb_connection_t* connection, xcb_screen_t* screen,
            xcb_window_t windowid, xcb_window_t parentid, EGLDisplay egl_display, const Image& image);
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
    EGLDisplay egl_display;
    const Image& image;
};

#endif
