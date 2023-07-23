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

#include <spdlog/spdlog.h>

constexpr auto config_attrs = std::to_array<EGLint>({
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

const auto context_attrs = std::to_array<EGLint>({
    EGL_CONTEXT_MAJOR_VERSION, 4,
    EGL_CONTEXT_MINOR_VERSION, 6,
#ifdef DEBUG
    EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
    EGL_CONTEXT_OPENGL_ROBUST_ACCESS, EGL_TRUE,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
    EGL_NONE
});

void GLAPIENTRY debug_callback(
            [[maybe_unused]] GLenum source,
            [[maybe_unused]] GLenum type,
            [[maybe_unused]] GLuint gl_id, GLenum severity,
            [[maybe_unused]] GLsizei length, const GLchar* message,
            [[maybe_unused]] const void* user)
{
    if (type != GL_DEBUG_TYPE_ERROR) {
        return;
    }
    const auto logger = spdlog::get("opengl");
    logger->error("Type: {:#X}, Severity: {:#X}, Message: {}", type, severity, message);
}

template <class T, class V>
EGLUtil<T, V>::EGLUtil(EGLenum platform, T* native_display):
display(eglGetPlatformDisplay(platform, native_display, nullptr))
{
    logger = spdlog::get("opengl");

    if (display == EGL_NO_DISPLAY) {
        logger->error("Could not obtain display, error {:#X}", eglGetError());
        throw std::runtime_error("");
    }

    EGLBoolean eglres = eglInitialize(display, nullptr, nullptr);
    if (eglres != EGL_TRUE) {
        logger->error("Could not initialize display, error {:#X}", eglGetError());
        throw std::runtime_error("");
    }

    eglres = eglBindAPI(EGL_OPENGL_API);
    if (eglres != EGL_TRUE) {
        logger->error("Could not bind to OpenGL API, error {:#X}", eglGetError());
        throw std::runtime_error("");
    }

    int num_config = 0;
    eglres = eglChooseConfig(display, config_attrs.data(), &config, 1, &num_config);
    if (eglres != EGL_TRUE || num_config != 1) {
        logger->error("Could not create config, error {:#X}", eglGetError());
        throw std::runtime_error("");
    }

    logger->info("Using EGL 1.5 and OpenGL 4.6");
}

template <class T, class V>
EGLUtil<T, V>::~EGLUtil()
{
    eglTerminate(display);
}

template <class T, class V>
auto EGLUtil<T, V>::create_context(EGLSurface surface) -> EGLContext
{
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrs.data());
    if (context == EGL_NO_CONTEXT) {
        logger->error("Could not create context, error {:#X}", eglGetError());
        return context;
    }

#ifdef DEBUG
    run_contained(surface, context, [] {
        glDebugMessageCallback(debug_callback, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    });
#endif

    return context;
}

template <class T, class V>
auto EGLUtil<T, V>::create_surface(V* native_window) -> EGLSurface
{
    EGLSurface surface = eglCreatePlatformWindowSurface(display, config, native_window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        logger->error("Could not create surface, error {:#X}", eglGetError());
    }
    return surface;
}

template <class T, class V>
void EGLUtil<T, V>::make_current(EGLSurface surface, EGLContext context)
{
    eglMakeCurrent(display, surface, surface, context);
}

template <class T, class V>
void EGLUtil<T, V>::restore()
{
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

template <class T, class V>
void EGLUtil<T, V>::run_contained(EGLSurface surface, EGLContext context, const std::function<void()>& func)
{
    make_current(surface, context);
    func();
    restore();
}

template <class T, class V>
void EGLUtil<T, V>::get_texture_from_image(const Image& image, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA8, image.width(), image.height(), 0,
            GL_BGRA, GL_UNSIGNED_BYTE, image.data());
}

#ifdef ENABLE_X11
#include <xcb/xcb.h>
template class EGLUtil<xcb_connection_t, xcb_window_t>;
#endif

#ifdef ENABLE_WAYLAND
#include <wayland-egl.h>
#include <wayland-client.h>
template class EGLUtil<struct wl_display, struct wl_egl_window>;
#endif
