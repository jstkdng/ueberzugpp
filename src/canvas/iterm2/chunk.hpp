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

#ifndef __ITERM2_CHUNK__
#define __ITERM2_CHUNK__

#include <vector>
#include <memory>
#include <cstdint>

class Iterm2Chunk
{
public:
    Iterm2Chunk() = default;
    explicit Iterm2Chunk(uint64_t size);

    void operator()(std::unique_ptr<Iterm2Chunk>& chunk) const;
    void static process_chunk(std::unique_ptr<Iterm2Chunk>& chunk);

    [[nodiscard]] auto get_size() const -> uint64_t;
    void set_size(uint64_t size);
    auto get_buffer() -> char*;
    auto get_result() -> char*;

private:
    uint64_t size;
    std::vector<char> buffer;
    std::vector<char> result;

};

#endif
