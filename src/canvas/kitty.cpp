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
    const int num_chunks = image->size() / 4096;
    const int last_chunk_size = image->size() % 4096;
    const int initial_chunk_size = (num_chunks > 0) ? 4096 : last_chunk_size;

    // initial data chunk
    ss  << "\e_G";
    if (num_chunks > 0) ss << "m=1";
    ss  << ",i=1337,q=2"
        << ",f=" << image->channels() * 8
        << ",s=" << image->width()
        << ",v=" << image->height()
        << ";" << util::base64_encode_ssl(image->data(), initial_chunk_size)
        << "\e\\";

    // regular chunks
    auto ptr = image->data() + 4096;
    for (int i = 0; i < num_chunks - 1; ++i, ptr += 4096) {
        ss  << "\e_Gm=1,q=2;"
            << util::base64_encode_ssl(ptr, 4096)
            << "\e\\";
    }

    // final data chunk
    if (num_chunks > 0 && last_chunk_size != 0) {
        ss  << "\e_Gm=0,q=2;"
            << util::base64_encode_ssl(ptr, last_chunk_size)
            << "\e\\";
    }

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << ss.rdbuf() << "\e_Ga=p,i=1337,q=2;\e\\" << std::flush;
    util::restore_cursor_position();
}

void KittyCanvas::clear()
{
    std::cout << "\e_Ga=d\e\\" << std::flush;
}
