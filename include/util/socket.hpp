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

#ifndef UTIL_SOCKET_H
#define UTIL_SOCKET_H

#include <string_view>
#include <cstdint>
#include <string>
#include <vector>

class UnixSocket
{
public:
    UnixSocket();
    explicit UnixSocket(std::string_view endpoint);
    ~UnixSocket();

    void connect_to_endpoint(std::string_view endpoint);
    void bind_to_endpoint(std::string_view endpoint) const;
    [[nodiscard]] auto wait_for_connections(int waitms) const -> int;
    [[nodiscard]] auto read_data_from_connection(int filde) -> std::vector<std::string>;
    void write(const void* data, std::size_t len) const;
    void read(void* data, std::size_t len) const;
private:
    int fd;
    bool connected = true;
    std::string buffer;
};

#endif
