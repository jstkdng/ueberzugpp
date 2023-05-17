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
#include "chunk.hpp"
#include "util.hpp"

#include <iostream>
#ifndef __APPLE__
#   include <execution>
#else
#   include <oneapi/tbb.h>
#endif

KittyCanvas::KittyCanvas()
{
    logger = spdlog::get("kitty");
    logger->info("Canvas created");
}

void KittyCanvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    image = std::move(new_image);
    x = dimensions.x + 1;
    y = dimensions.y + 1;
}

void KittyCanvas::draw()
{
    draw_frame();
}

auto KittyCanvas::process_chunks() -> std::vector<KittyChunk>
{
    const uint64_t chunk_size = 4096;
    uint64_t num_chunks = image->size() / chunk_size;
    uint64_t last_chunk_size = image->size() % chunk_size;
    if (last_chunk_size == 0) {
        last_chunk_size = chunk_size;
        num_chunks--;
    }

    std::vector<KittyChunk> chunks;
    chunks.reserve(num_chunks + 1);
    const auto *ptr = image->data();

    uint64_t idx = 0;
    for (; idx < num_chunks; idx++) {
        chunks.emplace_back(ptr + idx * chunk_size, chunk_size);
    }
    chunks.emplace_back(ptr + idx * chunk_size, last_chunk_size);

#ifdef __APPLE__
    oneapi::tbb::parallel_for_each(std::begin(chunks), std::end(chunks), KittyChunk());
#else
    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), KittyChunk::process_chunk);
#endif
    return chunks;
}

void KittyCanvas::draw_frame()
{
    const int bits_per_channel = 8;
    auto chunks = process_chunks();
    ss  << "\033_Ga=T,m=1,i=1337,q=2"
        << ",f=" << image->channels() * bits_per_channel
        << ",s=" << image->width()
        << ",v=" << image->height()
        << ";" << chunks.front().get_result()
        << "\033\\";

    for (auto chunk = std::next(std::begin(chunks)); chunk != std::prev(std::end(chunks)); std::advance(chunk, 1)) {
        ss  << "\033_Gm=1,q=2;"
            << chunk->get_result()
            << "\033\\";
    }

    ss  << "\033_Gm=0,q=2;"
        << chunks.back().get_result()
        << "\033\\";

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << ss.rdbuf() << std::flush;
    util::restore_cursor_position();
}

void KittyCanvas::clear()
{
    std::cout << "\033_Ga=d\033\\" << std::flush;
    image.reset();
}
