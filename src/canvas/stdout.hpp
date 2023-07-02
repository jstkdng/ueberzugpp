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

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

template<WindowType T>
class StdoutCanvas : public Canvas
{
public:
    StdoutCanvas()
    {
        stdout_mutex = std::make_shared<std::mutex>();
    }

    ~StdoutCanvas() override = default;

    void add_image(const std::string& identifier, std::unique_ptr<Image> new_image) override
    {
        remove_image(identifier);
        images.insert({identifier, std::make_unique<T>(std::move(new_image), stdout_mutex)});
        images.at(identifier)->draw();
    }

    void remove_image(const std::string& identifier) override
    {
        images.erase(identifier);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<T>> images;
    std::shared_ptr<std::mutex> stdout_mutex;
};

#endif
