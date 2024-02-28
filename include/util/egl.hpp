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

#include "image.hpp"

#include <functional>
#include <memory>

#include <spdlog/fwd.h>

#include <EGL/egl.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

template <class T, class V>
class EGLUtil
{
  public:
    EGLUtil(EGLenum platform, T *native_display, const EGLAttrib *attrib = nullptr);
    ~EGLUtil();

    void get_texture_from_image(const Image &image, GLuint texture) const;
    void run_contained(EGLSurface surface, EGLContext context, const std::function<void()> &func) const;
    void make_current(EGLSurface surface, EGLContext context) const;
    void restore() const;
    [[nodiscard]] auto create_surface(V *native_window) const -> EGLSurface;
    [[nodiscard]] auto create_context(EGLSurface surface) const -> EGLContext;

    EGLDisplay display;

  private:
    EGLConfig config;
    std::shared_ptr<spdlog::logger> logger;

    [[nodiscard]] auto error_to_string() const -> std::string;
};

#endif
