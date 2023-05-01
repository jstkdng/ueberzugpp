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
#include "util.hpp"
#include "fstream"

#include <iostream>
#include <execution>
#include <filesystem>

namespace fs = std::filesystem;

struct Iterm2Chunk
{
    long size;
    std::vector<char> buffer;
    std::unique_ptr<unsigned char[]> result {nullptr};

    Iterm2Chunk(long size) {
        buffer.resize(size, 0);
    };
};

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
    ss << "\e]1337;File=inline=1;";
    auto filename = image->filename();
    auto num_bytes = fs::file_size(filename);
    auto encoded_filename = util::base64_encode(
            reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size());
    ss << "size=" << num_bytes
        << ";name=" << encoded_filename
        << ";width=" << image->width() << "px"
        << ";height=" << image->height() << "px"
        << ":";

    auto chunks = process_chunks(filename, 2046, num_bytes);
    for (const auto& chunk: chunks) {
        ss << chunk->result;
    }

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
        ifs.read(chunk->buffer.data(), chunk_size);
        chunk->size = ifs.gcount();
        chunks.push_back(std::move(chunk));
    }

    auto chunk_processor = [](std::unique_ptr<Iterm2Chunk>& chunk) -> void
    {
        size_t bufsize = 4*((chunk->size+2)/3);
        chunk->result = std::make_unique<unsigned char[]>(bufsize + 1);
        util::base64_encode_v2(reinterpret_cast<unsigned char*>(chunk->buffer.data()),
                chunk->size, chunk->result.get());
    };

    std::for_each(std::execution::par_unseq, std::begin(chunks), std::end(chunks), chunk_processor);
    return chunks;
}
