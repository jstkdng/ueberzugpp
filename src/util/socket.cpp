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

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "os.hpp"
#include "util.hpp"

namespace fs = std::filesystem;

UnixSocket::UnixSocket(const std::string_view endpoint)
    : fd(socket(AF_UNIX, SOCK_STREAM, 0))
{
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category());
    }
    connect_to_endpoint(endpoint);
}

UnixSocket::UnixSocket()
    : fd(socket(AF_UNIX, SOCK_STREAM, 0))
{
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category());
    }
    const int bufsize = 8192;
    buffer.reserve(bufsize);
}

void UnixSocket::connect_to_endpoint(const std::string_view endpoint)
{
    if (!fs::exists(endpoint)) {
        connected = false;
        return;
    }
    struct sockaddr_un sock;
    std::memset(&sock, 0, sizeof(sockaddr_un));
    sock.sun_family = AF_UNIX;
    endpoint.copy(sock.sun_path, endpoint.size());

    int res = connect(fd, reinterpret_cast<const struct sockaddr *>(&sock), sizeof(struct sockaddr_un));
    if (res == -1) {
        throw std::system_error(errno, std::generic_category());
    }
}

void UnixSocket::bind_to_endpoint(const std::string_view endpoint) const
{
    struct sockaddr_un sock;
    std::memset(&sock, 0, sizeof(sockaddr_un));
    sock.sun_family = AF_UNIX;
    endpoint.copy(sock.sun_path, endpoint.size());

    int res = bind(fd, reinterpret_cast<const struct sockaddr *>(&sock), sizeof(struct sockaddr_un));
    if (res == -1) {
        throw std::system_error(errno, std::generic_category());
    }
    res = listen(fd, SOMAXCONN);
    if (res == -1) {
        throw std::system_error(errno, std::generic_category());
    }
}

auto UnixSocket::wait_for_connections(int waitms) const -> int
{
    const auto in_event = os::wait_for_data_on_fd(fd, waitms);
    if (in_event) {
        return accept(fd, nullptr, nullptr);
    }
    return -1;
}

auto UnixSocket::read_data_from_connection(int filde) -> std::vector<std::string>
{
    // a single connection could send multiples commands at once
    // each command should end with a '\n' character
    const int read_buffer_size = 4096;
    std::array<char, read_buffer_size> read_buffer;
    while (true) {
        const auto status = recv(filde, read_buffer.data(), read_buffer_size, 0);
        if (status <= 0) {
            break;
        }
        buffer.append(read_buffer.data(), status);
    }
    auto cmds = util::str_split(buffer, "\n");
    buffer.clear();
    close(filde);
    return cmds;
}

void UnixSocket::write(const void *data, std::size_t len) const
{
    if (!connected) {
        return;
    }
    const auto *runner = static_cast<const uint8_t *>(data);
    while (len != 0) {
        const auto status = send(fd, runner, len, MSG_NOSIGNAL);
        if (status == -1) {
            throw std::system_error(errno, std::generic_category());
        }
        len -= status;
        runner += status;
    }
}

void UnixSocket::read(void *data, std::size_t len) const
{
    if (!connected) {
        return;
    }
    auto *runner = static_cast<uint8_t *>(data);
    while (len != 0) {
        const auto status = recv(fd, runner, len, 0);
        if (status == 0) {
            return; // no data
        }
        if (status == -1) {
            throw std::system_error(errno, std::generic_category());
        }
        len -= status;
        runner += status;
    }
}

auto UnixSocket::read_until_empty() const -> std::string
{
    std::string result;
    const int read_buffer_size = 4096;
    std::array<char, read_buffer_size> read_buffer;
    result.reserve(read_buffer_size);
    while (true) {
        const auto status = recv(fd, read_buffer.data(), read_buffer_size, 0);
        if (status <= 0) {
            break;
        }
        result.append(read_buffer.data(), status);
    }
    return result;
}

UnixSocket::~UnixSocket()
{
    close(fd);
}
