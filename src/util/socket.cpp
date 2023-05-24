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

#include "util/socket.hpp"

#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

UnixSocket::UnixSocket(const std::string_view endpoint):
fd(socket(AF_UNIX, SOCK_STREAM, 0))
{
    if (fd == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
    struct sockaddr_un sock;
    std::memset(&sock, 0, sizeof(sockaddr_un));
    sock.sun_family = AF_UNIX;
    endpoint.copy(sock.sun_path, endpoint.size());

    int res = connect(fd, reinterpret_cast<const struct sockaddr*>(&sock),
            sizeof(struct sockaddr_un));
    if (res == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void UnixSocket::write(const void* data, std::size_t len) const
{
    const auto *runner = reinterpret_cast<const uint8_t*>(data);
    while (len != 0) {
        const auto status = send(fd, runner, len, MSG_NOSIGNAL);
        if (status == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
        len -= status;
        runner += status;
    }
}

void UnixSocket::read(void* data, std::size_t len) const
{
    auto *runner = reinterpret_cast<uint8_t*>(data);
    while (len != 0) {
        const auto status = recv(fd, runner, len, 0);
        if (status == 0) {
            return; // no data
        }
        if (status == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
        len -= status;
        runner += status;
    }
}

UnixSocket::~UnixSocket()
{
    close(fd);
}
