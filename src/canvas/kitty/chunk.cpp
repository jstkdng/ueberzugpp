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

KittyChunk::KittyChunk(const unsigned char* ptr, uint64_t size):
ptr(ptr),
size(size)
{
    uint64_t bufsize = 4*((size+2)/3);
    result.resize(bufsize + 1, 0);
}

void KittyChunk::process_chunk(KittyChunk& chunk)
{
    util::base64_encode_v2(chunk.get_ptr(), chunk.get_size(),
            reinterpret_cast<unsigned char*>(chunk.get_result()));
}

void KittyChunk::operator()(KittyChunk& chunk) const
{
    process_chunk(chunk);
}

auto KittyChunk::get_result() -> char*
{
    return result.data();
}

auto KittyChunk::get_ptr() const -> const unsigned char*
{
    return ptr;
}

auto KittyChunk::get_size() const -> uint64_t
{
    return size;
}
