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

#ifndef __KITTY_CANVAS__
#define __KITTY_CANVAS__

#include "canvas.hpp"

#include <string>
#include <unordered_map>
#include <mutex>
#include <spdlog/spdlog.h>

class Window;

class KittyCanvas : public Canvas
{
public:
    KittyCanvas();
    ~KittyCanvas() override = default;

    void add_image(const std::string& identifier, std::unique_ptr<Image> new_image) override;
    void remove_image(const std::string& identifier) override;

private:
    std::unordered_map<std::string, std::unique_ptr<Window>> images;
    std::shared_ptr<spdlog::logger> logger;
    std::mutex stdout_mutex;
};

#endif
