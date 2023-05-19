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

#ifndef __KITTY_CHUNK__
#define __KITTY_CHUNK__

#include <vector>
#include <cstdint>

class KittyChunk
{
public:
    KittyChunk() = default;
    KittyChunk(const unsigned char* ptr, uint64_t size);

    void operator()(KittyChunk& chunk) const;

    auto get_result() -> char*;
    [[nodiscard]] auto get_ptr() const -> const unsigned char*;
    [[nodiscard]] auto get_size() const -> uint64_t;

    static void process_chunk(KittyChunk& chunk);
private:
    const unsigned char* ptr;
    uint64_t size;
    std::vector<char> result;
};

#endif
