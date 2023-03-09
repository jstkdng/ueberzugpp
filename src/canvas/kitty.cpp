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

#include "kitty.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>

KittyCanvas::KittyCanvas()
{}

KittyCanvas::~KittyCanvas()
{}

void KittyCanvas::init(const Dimensions& dimensions, std::shared_ptr<Image> image)
{
    this->image = image;
    x = dimensions.x + 1;
    y = dimensions.y + 1;
}

void KittyCanvas::draw()
{
    draw_frame();
}

void KittyCanvas::draw_frame()
{
    std::stringstream ss;
    int num_chunks = image->size() / 4096;
    int last_cunk_size = image->size() % 4096;

    // initial data chunk
    ss  << "\e_Gm=1,i=1337,q=2"
        << ",f=" << image->channels() * 8
        << ",s=" << image->width()
        << ",v=" << image->height()
        << ";" << util::base64_encode(image->data(), 4096)
        << "\e\\";

    // regular chunks
    auto ptr = image->data() + 4096;
    for (int i = 0; i < num_chunks - 1; ++i, ptr += 4096) {
        ss  << "\e_Gm=1,q=2;"
            << util::base64_encode(ptr, 4096)
            << "\e\\";
    }

    // final data chunk
    ss  << "\e_Gm=0,q=2;"
        << util::base64_encode(ptr, last_cunk_size)
        << "\e\\";

    util::move_cursor(y, x);
    std::cout << ss.rdbuf() << "\e_Ga=p,i=1337,q=2;\e\\" << std::flush;
}

void KittyCanvas::clear()
{
    std::cout << "\e_Ga=d\e\\" << std::flush;
}
