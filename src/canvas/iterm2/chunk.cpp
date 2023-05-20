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

#include "chunk.hpp"
#include "util.hpp"

Iterm2Chunk::Iterm2Chunk(uint64_t size)
{
    auto bufsize = 4*((size+2)/3);
    buffer.resize(size, 0);
    result.resize(bufsize + 1, 0);
}

void Iterm2Chunk::set_size(uint64_t size)
{
    this->size = size;
}

auto Iterm2Chunk::get_size() const -> uint64_t
{
    return size;
}

auto Iterm2Chunk::get_buffer() -> char*
{
    return buffer.data();
}

auto Iterm2Chunk::get_result() -> char*
{
    return result.data();
}

void Iterm2Chunk::process_chunk(std::unique_ptr<Iterm2Chunk>& chunk)
{
    util::base64_encode_v2(reinterpret_cast<unsigned char*>(chunk->get_buffer()),
            chunk->get_size(), reinterpret_cast<unsigned char*>(chunk->get_result()));
}

void Iterm2Chunk::operator()(std::unique_ptr<Iterm2Chunk>& chunk) const
{
    process_chunk(chunk);
}
