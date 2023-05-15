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

void Iterm2Canvas::init(const Dimensions& dimensions, std::shared_ptr<Image> image)
{
    this->image = image;
    x = dimensions.x + 1;
    y = dimensions.y + 1;
    max_width = dimensions.max_w;
    max_height = dimensions.max_h;
}

void Iterm2Canvas::draw()
{
    ss << "\033]1337;File=inline=1;";
    auto filename = image->filename();
    auto num_bytes = fs::file_size(filename);
    auto encoded_filename = util::base64_encode(
            reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size());
    ss << "size=" << num_bytes
        << ";name=" << encoded_filename
        << ";width=" << image->width() << "px"
        << ";height=" << image->height() << "px"
        << ":";

    const int chunk_size = 1023;
    auto chunks = process_chunks(filename, chunk_size, num_bytes);

    auto print_result = [&] (std::unique_ptr<Iterm2Chunk>& chunk) -> void
    {
        ss << chunk->get_result();
    };
    std::for_each(std::begin(chunks), std::end(chunks), print_result);
    ss << "\a";

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << ss.rdbuf() << std::flush;
    util::restore_cursor_position();
}

void Iterm2Canvas::clear()
{
    util::clear_terminal_area(x, y, max_width, max_height);
}

auto Iterm2Canvas::process_chunks(const std::string& filename, int chunk_size, size_t num_bytes) -> std::vector<std::unique_ptr<Iterm2Chunk>>
{
    int num_chunks = std::ceil(static_cast<double>(num_bytes) / chunk_size);
    std::vector<std::unique_ptr<Iterm2Chunk>> chunks;
    chunks.reserve(num_chunks);

    std::ifstream ifs (filename);
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
