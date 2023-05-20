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

#include <fmt/format.h>
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
    const uint64_t bytes_per_chunk = 4*((chunk_size+2)/3) + 100;
    str.reserve((num_chunks + 2) * bytes_per_chunk);

    std::vector<KittyChunk> chunks;
    chunks.reserve(num_chunks + 2);
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
    str.append(fmt::format("\033_Ga=T,m=1,i=1337,q=2,f={},s={},v={};{}\033\\",
            image->channels() * bits_per_channel, image->width(),
            image->height(), chunks.front().get_result()));

    for (auto chunk = std::next(std::begin(chunks)); chunk != std::prev(std::end(chunks)); std::advance(chunk, 1)) {
        str.append(fmt::format("\033_Gm=1,q=2;{}\033\\", chunk->get_result()));
    }

    str.append(fmt::format("\033_Gm=0,q=2;{}\033\\", chunks.back().get_result()));

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
    str.clear();
}

void KittyCanvas::clear()
{
    std::cout << "\033_Ga=d\033\\" << std::flush;
    image.reset();
}
