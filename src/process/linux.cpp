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

#include "process.hpp"

#include <fmt/format.h>
#include <fstream>
#include <limits>
#ifdef __FreeBSD__
#  include <sys/types.h>
#else
#  include <sys/sysmacros.h>
#endif

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

Process::Process(int pid)
    : pid(pid)
{
    const auto stat = fmt::format("/proc/{}/stat", pid);
    std::ifstream ifs(stat);
    ifs.ignore(max_size, ')'); // skip pid and executable name
    ifs >> state >> ppid >> process_group_id >> session_id >> tty_nr;
    minor_dev = minor(tty_nr); // NOLINT
    pty_path = fmt::format("/dev/pts/{}", minor_dev);
}
