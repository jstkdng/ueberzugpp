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

#ifndef UTIL_EGL_H
#define UTIL_EGL_H

#include <EGL/egl.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

class Image;

class EGLUtil
{
public:
    explicit EGLUtil(EGLDisplay display);
    ~EGLUtil();

    static auto get_texture_from_image(const Image& image) -> GLuint;
    static void GLAPIENTRY debug_callback(GLenum source, GLenum type,
            GLuint gl_id, GLenum severity, GLsizei length, const GLchar* message, const void* user);

    EGLContext context;
    EGLConfig config;

private:
    void set_context();
    void set_config();

    EGLDisplay display;
};

#endif
