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

#ifndef __SWAY_CANVAS__
#define __SWAY_CANVAS__

#include "canvas.hpp"
#include "dimensions.hpp"
#include "terminal.hpp"
#include "shm.hpp"

#include <wayland-client.h>
#include <memory>

class SwayCanvas : public Canvas
{
public:
    explicit SwayCanvas();
    ~SwayCanvas() override;

    static void registry_handle_global(void *data, wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version);
    static void registry_handle_global_remove(void *data, wl_registry *registry,
        uint32_t name);

    void init(const Dimensions& dimensions, std::unique_ptr<Image> new_image) override;
    void draw() override;
    void clear() override;

    wl_compositor *compositor = nullptr;
    std::unique_ptr<SwayShm> shm;

private:
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;

    int x;
    int y;

    std::unique_ptr<Image> image;
};

#endif
