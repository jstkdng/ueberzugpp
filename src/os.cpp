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

#include "os.hpp"

#include <cstdlib>
#include <array>
#include <memory>
#include <system_error>
#include <iostream>

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <poll.h>

auto os::exec(const std::string_view cmd) -> std::string
{
    const int bufsize = 128;
    std::array<char, bufsize> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
    if (!pipe) {
        throw std::system_error(errno, std::generic_category());
    }
    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty()) {
        result.erase(result.length() - 1);
    }
    return result;
}

auto os::read_data_from_fd(int filde) -> std::string
{
    const int bufsize = 128;
    std::string response;
    std::array<char, bufsize> buffer;

    while (true) {
        const auto status = read(filde, buffer.data(), bufsize);
        if (status == -1) {
            throw std::system_error(errno, std::generic_category());
        }
        if (status == 0) {
            break;
        }
        response.append(buffer.data(), status);
        if (buffer[status - 1] == '\n' && filde == STDIN_FILENO) {
            break;
        }
    }
    return response;
}

auto os::read_data_from_stdin() -> std::string
{
    auto response = read_data_from_fd(STDIN_FILENO);
    response.pop_back();
    return response;
}

auto os::wait_for_data_on_fd(int filde, int waitms) -> bool
{
    struct pollfd conn;
    std::memset(&conn, 0, sizeof(struct pollfd));
    conn.fd = filde;
    conn.events = POLLIN;

    const int res = poll(&conn, 1, waitms);
    if (res == -1 && errno != EINTR) {
        throw std::system_error(errno, std::generic_category());
    }
    return (conn.revents & POLLIN) != 0;
}

auto os::wait_for_data_on_stdin(int waitms) -> bool
{
    return wait_for_data_on_fd(STDIN_FILENO, waitms);
}

auto os::getenv(const std::string_view var) -> std::optional<std::string>
{
    auto *res = std::getenv(var.data());
    if (res == nullptr) {
        return {};
    }
    return res;
}

auto os::get_pid() -> int
{
    return getpid();
}

auto os::get_ppid() -> int
{
    return getppid();
}

auto os::fork_process() -> int
{
    return fork();
}

auto os::create_new_session() -> int
{
    return setsid();
}
