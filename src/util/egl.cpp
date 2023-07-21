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

#include "util/egl.hpp"
#include "image.hpp"

#include <iostream>

void GLAPIENTRY EGLUtil::debug_callback(
            [[maybe_unused]] GLenum source,
            [[maybe_unused]] GLenum type,
            [[maybe_unused]] GLuint gl_id, GLenum severity,
            [[maybe_unused]] GLsizei length, const GLchar* message,
            [[maybe_unused]] const void* user)
{
    if (type != GL_DEBUG_TYPE_ERROR) {
        return;
    }
    std::cout << "[OpenGL Error](" << type << ")[" << severity << "] "
        << message << std::endl;
}

EGLUtil::EGLUtil(EGLDisplay display):
display(display)
{
    set_config();
    set_context();
}

EGLUtil::~EGLUtil()
{
    eglDestroyContext(display, context);
}

void EGLUtil::set_config()
{
    constexpr auto attrs = std::to_array<EGLint>({
        EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
        EGL_CONFORMANT,        EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
    
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
    
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE,  8,
    
        EGL_NONE
    });
    int num_config = 0;
    auto eglres = eglChooseConfig(display, attrs.data(), &config, 1, &num_config);
    if (eglres != EGL_TRUE || num_config != 1) {
        std::cout << "Could not create config" << std::endl;
    }
}

void EGLUtil::set_context()
{
    const auto attrs = std::to_array<EGLint>({
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 6,
#ifdef DEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
        EGL_CONTEXT_OPENGL_ROBUST_ACCESS, EGL_TRUE,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    });
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs.data());
    if (context == EGL_NO_CONTEXT) {
        std::cout << "Could not create context" << std::endl;
    }
}

auto EGLUtil::get_texture_from_image(const Image& image) -> GLuint
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA8, image.width(), image.height(), 0,
            GL_BGRA, GL_UNSIGNED_BYTE, image.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}
