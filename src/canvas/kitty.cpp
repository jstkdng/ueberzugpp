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
#ifndef __APPLE__
#   include <execution>
#else
#   include <oneapi/tbb.h>
#endif

struct KittyChunk
{
    const unsigned char* ptr;
    int size;
    std::unique_ptr<unsigned char[]> result {nullptr};

    KittyChunk(const unsigned char* ptr, int size):
        ptr(ptr), size(size) {}
};

void chunk_processor(KittyChunk& chunk)
{
    size_t bufsize = 4*((chunk.size+2)/3);
    chunk.result = std::make_unique<unsigned char[]>(bufsize + 1);
    util::base64_encode_v2(chunk.ptr, chunk.size, chunk.result.get());
};

class ProcessKittyChunk
{
public:
    void operator()(KittyChunk& chunk) const {
        chunk_processor(chunk);
    }
};

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

auto KittyCanvas::process_chunks() -> std::vector<KittyChunk>
{
    int num_chunks = image->size() / 4096;
    int last_chunk_size = image->size() % 4096;
    if (!last_chunk_size) {
        last_chunk_size = 4096;
        num_chunks--;
    }

    std::vector<KittyChunk> chunks;
    chunks.reserve(num_chunks + 1);
    auto ptr = image->data();

    int i;
    for (i = 0; i < num_chunks; i++) {
        chunks.emplace_back(ptr + i * 4096, 4096);
    }
    chunks.emplace_back(ptr + i * 4096, last_chunk_size);

#ifdef __APPLE__
    oneapi::tbb::parallel_for_each(std::begin(chunks), std::end(chunks), ProcessKittyChunk());
#else
    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), chunk_processor);
#endif
    return chunks;
}

void KittyCanvas::draw_frame()
{
    auto chunks = process_chunks();
    ss  << "\e_Ga=T,m=1,i=1337,q=2"
        << ",f=" << image->channels() * 8
        << ",s=" << image->width()
        << ",v=" << image->height()
        << ";" << chunks.front().result
        << "\e\\";

    for (auto chunk = std::next(std::begin(chunks)); chunk != std::prev(std::end(chunks)); std::advance(chunk, 1)) {
        ss  << "\e_Gm=1,q=2;"
            << chunk->result
            << "\e\\";
    }

    ss  << "\e_Gm=0,q=2;"
        << chunks.back().result
        << "\e\\";

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << ss.rdbuf() << std::flush;
    util::restore_cursor_position();
}

void KittyCanvas::clear()
{
    std::cout << "\e_Ga=d\e\\" << std::flush;
}
