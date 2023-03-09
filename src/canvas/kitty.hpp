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

class KittyCanvas : public Canvas
{
public:
    KittyCanvas();
    ~KittyCanvas();

    void init(const Dimensions& dimensions, std::shared_ptr<Image> image) override;
    void draw() override;
    void clear() override;

private:
    std::shared_ptr<Image> image;
    std::string encoded_img;

    int x;
    int y;
};

#endif
