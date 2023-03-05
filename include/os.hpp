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

#ifndef __NAMESPACE_OS__
#define __NAMESPACE_OS__

#include <optional>
#include <string>
#include <string_view>

namespace os
{
    std::string exec(std::string_view cmd);
    auto getenv(std::string_view var) -> std::optional<std::string>;

    int get_pid();

    void get_process_info(int pid);
}

#endif
