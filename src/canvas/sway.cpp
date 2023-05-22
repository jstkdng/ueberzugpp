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

#include "sway.hpp"
//#include "wayland-xdg-shell-client-protocol.h"

#include <iostream>
#include <array>
#include <string_view>
#include <fmt/format.h>

void SwayCanvas::registry_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    std::string_view interface_str { interface };
    auto *canvas = reinterpret_cast<SwayCanvas*>(data);
    if (interface_str == wl_compositor_interface.name) {
        canvas->compositor = reinterpret_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, version)
        );
    } else if (interface_str == wl_shm_interface.name) {
        canvas->shm = reinterpret_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, version)
        );
    }
}

void SwayCanvas::registry_handle_global_remove(void *data, struct wl_registry *registry,
        uint32_t name)
{
    std::ignore = data;
    std::ignore = registry;
    std::ignore = name;
}

constexpr struct wl_registry_listener registry_listener = {
    .global = SwayCanvas::registry_handle_global,
    .global_remove = SwayCanvas::registry_handle_global_remove
};

SwayCanvas::SwayCanvas():
display(wl_display_connect(nullptr))
{
    if (display == nullptr) {
        throw std::runtime_error("Failed to connect to wayland display.");
    }
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);
}

SwayCanvas::~SwayCanvas()
{
    wl_display_disconnect(display);
}

void SwayCanvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    image = std::move(new_image);
    x = dimensions.x;
    y = dimensions.y;
}

void SwayCanvas::draw()
{}

void SwayCanvas::clear()
{}
