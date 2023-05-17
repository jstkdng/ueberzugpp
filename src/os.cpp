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
#include <unistd.h>

auto os::exec(const std::string_view cmd) -> std::string
{
    const int bufsize = 128;
    std::array<char, bufsize> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed");
    }
    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty()) {
        result.erase(result.length() - 1);
    }
    return result;
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
