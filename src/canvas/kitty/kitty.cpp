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
#include "dimensions.hpp"
#include "util.hpp"

#include <fmt/format.h>

#include <iostream>

#ifdef HAVE_STD_EXECUTION_H
#  include <execution>
#else
#  include <oneapi/tbb.h>
#endif

Kitty::Kitty(std::unique_ptr<Image> new_image, std::mutex *stdout_mutex)
    : image(std::move(new_image)),
      stdout_mutex(stdout_mutex),
      id(util::generate_random_number<uint32_t>(1))
{
    const auto dims = image->dimensions();
    x = dims.x + 1;
    y = dims.y + 1;
}

Kitty::~Kitty()
{
    const std::scoped_lock lock{*stdout_mutex};
    std::cout << fmt::format("\033_Ga=d,d=i,i={}\033\\", id) << std::flush;
}

void Kitty::draw()
{
    generate_frame();
}

void Kitty::generate_frame()
{
    const int bits_per_channel = 8;
    auto chunks = process_chunks();
    str.append(fmt::format("\033_Ga=T,m=1,i={},q=2,f={},s={},v={};{}\033\\", id, image->channels() * bits_per_channel,
                           image->width(), image->height(), chunks.front().get_result()));

    for (auto chunk = std::next(std::begin(chunks)); chunk != std::prev(std::end(chunks)); std::advance(chunk, 1)) {
        str.append("\033_Gm=1,q=2;");
        str.append(chunk->get_result());
        str.append("\033\\");
    }

    str.append("\033_Gm=0,q=2;");
    str.append(chunks.back().get_result());
    str.append("\033\\");

    const std::scoped_lock lock{*stdout_mutex};
    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
    str.clear();
}

auto Kitty::process_chunks() -> std::vector<KittyChunk>
{
    const uint64_t chunk_size = 3068;
    uint64_t num_chunks = image->size() / chunk_size;
    uint64_t last_chunk_size = image->size() % chunk_size;
    if (last_chunk_size == 0) {
        last_chunk_size = chunk_size;
        num_chunks--;
    }
    const uint64_t bytes_per_chunk = 4 * ((chunk_size + 2) / 3) + 100;
    str.reserve((num_chunks + 2) * bytes_per_chunk);

    std::vector<KittyChunk> chunks;
    chunks.reserve(num_chunks + 2);
    const auto *ptr = image->data();

    uint64_t idx = 0;
    for (; idx < num_chunks; idx++) {
        chunks.emplace_back(ptr + idx * chunk_size, chunk_size);
    }
    chunks.emplace_back(ptr + idx * chunk_size, last_chunk_size);

#ifdef HAVE_STD_EXECUTION_H
    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), KittyChunk::process_chunk);
#else
    oneapi::tbb::parallel_for_each(std::begin(chunks), std::end(chunks), KittyChunk());
#endif

    return chunks;
}
