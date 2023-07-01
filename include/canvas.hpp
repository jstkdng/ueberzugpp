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

#ifndef __CANVAS__
#define __CANVAS__

#include <memory>

class Image;

class Canvas
{
public:
    static auto create() -> std::unique_ptr<Canvas>;
    virtual ~Canvas() = default;

    virtual void add_image(const std::string& identifier, std::unique_ptr<Image> new_image) = 0;
    virtual void remove_image(const std::string& identifier) = 0;

    virtual void show() {}
    virtual void hide() {}
    virtual void toggle() {}
};

#endif

