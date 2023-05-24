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

#ifndef __UTIL_SOCKET__
#define __UTIL_SOCKET__

#include <string_view>
#include <cstdint>

class UnixSocket
{
public:
    explicit UnixSocket(std::string_view endpoint);

    void write(const void* data, std::size_t len) const;
    void read(void* data, std::size_t len) const;

    ~UnixSocket();
private:
    int fd;
};

#endif
