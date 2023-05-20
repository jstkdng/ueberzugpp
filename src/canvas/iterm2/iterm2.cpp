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

#include "iterm2.hpp"
#include "chunk.hpp"
#include "util.hpp"
#include "fstream"

#include <iostream>
#include <filesystem>
#include <ranges>
#include <fmt/format.h>
#ifndef __APPLE__
#   include <execution>
#else
#   include <oneapi/tbb.h>
#endif

namespace fs = std::filesystem;

Iterm2Canvas::Iterm2Canvas()
{
    logger = spdlog::get("iterm2");
    logger->info("Canvas created");
}

void Iterm2Canvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    image = std::move(new_image);
    x = dimensions.x + 1;
    y = dimensions.y + 1;
    max_width = dimensions.max_w;
    max_height = dimensions.max_h;
    str = "";
}

void Iterm2Canvas::draw()
{
    str.append("\033]1337;File=inline=1;");
    const auto filename = image->filename();
    const auto num_bytes = fs::file_size(filename);
    const auto encoded_filename = util::base64_encode(
            reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size());
    str.append(fmt::format("size={};name={};width={}px;height={}px:",
                num_bytes, encoded_filename, image->width(), image->height()));

    const int chunk_size = 1023;
    const auto chunks = process_chunks(filename, chunk_size, num_bytes);
    const int num_chunks = std::ceil(static_cast<double>(num_bytes) / chunk_size);
    const uint64_t bytes_per_chunk = 4*((chunk_size+2)/3) + 100;
    str.reserve((num_chunks + 2) * bytes_per_chunk);

    std::ranges::for_each(std::as_const(chunks), [&] (const std::unique_ptr<Iterm2Chunk>& chunk) {
        str.append(chunk->get_result());
    });
    str.append("\a");

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
}

void Iterm2Canvas::clear()
{
    util::clear_terminal_area(x, y, max_width, max_height);
    image.reset();
}

auto Iterm2Canvas::process_chunks(const std::string_view filename, int chunk_size, size_t num_bytes) -> std::vector<std::unique_ptr<Iterm2Chunk>>
{
    const int num_chunks = std::ceil(static_cast<double>(num_bytes) / chunk_size);
    std::vector<std::unique_ptr<Iterm2Chunk>> chunks;
    chunks.reserve(num_chunks + 2);

    std::ifstream ifs (filename.data());
    while (ifs.good()) {
        auto chunk = std::make_unique<Iterm2Chunk>(chunk_size);
        ifs.read(chunk->get_buffer(), chunk_size);
        chunk->set_size(ifs.gcount());
        chunks.push_back(std::move(chunk));
    }

#ifdef __APPLE__
    oneapi::tbb::parallel_for_each(std::begin(chunks), std::end(chunks), Iterm2Chunk());
#else
    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), Iterm2Chunk::process_chunk);
#endif
    return chunks;
}
