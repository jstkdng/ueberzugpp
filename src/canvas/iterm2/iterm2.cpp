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
#include "dimensions.hpp"
#include "terminal.hpp"
#include "image.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <ranges>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#if !defined(__cpp_lib_execution) || (__cpp_lib_execution < 201902L)
#   include <oneapi/tbb.h>
#else
#   include <execution>
#endif

namespace fs = std::filesystem;

Iterm2::Iterm2(std::unique_ptr<Image> new_image, std::shared_ptr<std::mutex> stdout_mutex):
image(std::move(new_image)),
stdout_mutex(std::move(stdout_mutex))
{
    const auto dims = image->dimensions();
    x = dims.x + 1;
    y = dims.y + 1;
    horizontal_cells = std::ceil(static_cast<double>(image->width()) / dims.terminal.font_width);
    vertical_cells = std::ceil(static_cast<double>(image->height()) / dims.terminal.font_height);
    draw();
}

Iterm2::~Iterm2()
{
    std::scoped_lock lock {*stdout_mutex};
    util::clear_terminal_area(x, y, horizontal_cells, vertical_cells);
}

void Iterm2::draw()
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

    std::scoped_lock lock {*stdout_mutex};
    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
    str.clear();
}

auto Iterm2::process_chunks(const std::string_view filename, int chunk_size, size_t num_bytes) -> std::vector<std::unique_ptr<Iterm2Chunk>>
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

#if !defined(__cpp_lib_execution) || (__cpp_lib_execution < 201902L)
    oneapi::tbb::parallel_for_each(std::begin(chunks), std::end(chunks), Iterm2Chunk());
#else
    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), Iterm2Chunk::process_chunk);
#endif
    return chunks;
}
