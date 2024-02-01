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

#ifndef NAMESPACE_OS_H
#define NAMESPACE_OS_H

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace os
{

auto exec(std::string_view cmd) -> std::string;
auto getenv(std::string_view var) -> std::optional<std::string>;
auto read_data_from_fd(int filde, char sep = '\n') -> std::string;
auto read_data_from_stdin(char sep = '\n') -> std::string;
auto wait_for_data_on_fd(int filde, int waitms) -> bool;
auto wait_for_data_on_stdin(int waitms) -> bool;

auto get_pid() -> int;
auto get_ppid() -> int;

void get_process_info(int pid);
void daemonize();

} // namespace os

#endif
