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
#include "util/ptr.hpp"

#include <cstdlib>
#include <array>
#include <memory>
#include <system_error>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <poll.h>

const strmap envp {os::load_env()};

auto os::exec(const std::string_view cmd) -> std::string
{
    const int bufsize = 128;
    std::array<char, bufsize> buffer;
    std::string result;
    c_unique_ptr<FILE, pclose> pipe {popen(cmd.data(), "r")};
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

auto os::read_data_from_fd(int filde, char sep) -> std::string
{
    std::string response;
    char readch = 0;

    while (true) {
        const auto status = read(filde, &readch, 1);
        if (status == -1) {
            throw std::system_error(errno, std::generic_category());
        }
        if (status == 0) {
            if (response.empty()) {
                throw std::system_error(EIO, std::generic_category());
            }
            break;
        }
        if (readch == sep) {
            break;
        }
        response.push_back(readch);
    }
    return response;
}

auto os::read_data_from_stdin(char sep) -> std::string
{
    return read_data_from_fd(STDIN_FILENO, sep);
}

auto os::wait_for_data_on_fd(int filde, int waitms) -> bool
{
    struct pollfd fds;
    fds.fd = filde;
    fds.events = POLLIN;

    const int res = poll(&fds, 1, waitms);

    if (((fds.revents & POLLERR) != 0) ||
        ((fds.revents & POLLNVAL) != 0) ||
        (res == -1 && errno != EINTR)) {
        throw std::system_error(errno, std::generic_category());
    }
    // read all remaining data after a POLLHUP
    return (fds.revents & POLLIN) != 0 || (fds.revents & POLLHUP) != 0;
}

auto os::wait_for_data_on_stdin(int waitms) -> bool
{
    return wait_for_data_on_fd(STDIN_FILENO, waitms);
}

auto os::getenv(const std::string_view var) -> std::optional<std::string>
{
    const auto search = envp.find(var);
    if (search == envp.end()) {
        return {};
    }
    return search->second;
}

auto os::get_pid() -> int
{
    return getpid();
}

auto os::get_ppid() -> int
{
    return getppid();
}

void os::daemonize()
{
    const auto res = daemon(1, 1);
    if (res == -1) {
        throw std::system_error(errno, std::generic_category());
    }
}

// copy current environment
auto os::load_env() -> strmap
{
    char **runner = environ;
    strmap envp;
    while (*runner != nullptr) {
        const std::string_view envline {*runner};
        const auto idx = envline.find('=');
        const auto envname = envline.substr(0, idx);
        // check if variable is empty
        if (idx == envline.length() - 1) {
            envp.emplace(envname, "");
        } else {
            const auto envvalue = envline.substr(idx + 1);
            envp.emplace(envname, envvalue);
        }
        runner = runner + 1;
    }
    return envp;
}
