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

#ifndef __STDOUT_CANVAS__
#define __STDOUT_CANVAS__

#include "canvas.hpp"
#include "window.hpp"
#include "image.hpp"

#include <spdlog/spdlog.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

template<WindowType T>
class StdoutCanvas : public Canvas
{
public:
    explicit StdoutCanvas(const std::string& output)
    {
        stdout_mutex = std::make_shared<std::mutex>();
        logger = spdlog::get(output);
    }

    ~StdoutCanvas() override = default;

    void add_image(const std::string& identifier, std::unique_ptr<Image> new_image) override
    {
        logger->info("Displaying image with id {}", identifier);
        images.erase(identifier);
        images.insert({identifier, std::make_unique<T>(std::move(new_image), stdout_mutex)});
    }

    void remove_image(const std::string& identifier) override
    {
        logger->info("Removing image with id {}", identifier);
        images.erase(identifier);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<T>> images;
    std::shared_ptr<std::mutex> stdout_mutex;
    std::shared_ptr<spdlog::logger> logger;
};

#endif
