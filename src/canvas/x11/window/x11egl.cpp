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

constexpr auto surface_attrs = std::to_array<EGLAttrib>({
    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR, // or use EGL_GL_COLORSPACE_SRGB for sRGB framebuffer
    EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
    EGL_NONE,
});

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
            &windowid, surface_attrs.data());
    if (egl_surface == EGL_NO_SURFACE) {
        std::cout << "bruh3" << std::endl;
    }

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_util.context);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(EGLUtil::debug_callback, nullptr);

    std::array<char*, 1> buff;
    std::string vertex_shader_source = R"(
        #version 460 core
        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        }
    )";
    buff.at(0) = vertex_shader_source.data();

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, buff.data(), nullptr);
    glCompileShader(vertex_shader);

    std::string vertex_shader_source_2 = R"(
        #version 460 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
        } 
    )";
    buff.at(0) = vertex_shader_source_2.data();

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, buff.data(), nullptr);
    glCompileShader(fragment_shader);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    const auto vertices = std::to_array<GLfloat>({
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    });
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

X11EGLWindow::~X11EGLWindow()
{
    xcb_destroy_window(connection, windowid);
    xcb_flush(connection);

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
    eglSwapBuffers(egl_display, egl_surface);
}

void X11EGLWindow::generate_frame()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //glUseProgram(shader_program);
    //glBindVertexArray(vao);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
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
