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
#include "util.hpp"
#include "tmux.hpp"
#include "os.hpp"

#include <fstream>
#include <string>
#include <fmt/format.h>
#include <sys/sysmacros.h>

Process::Process(int pid):
pid(pid)
{
    const auto stat = fmt::format("/proc/{}/stat", pid);
    std::ifstream ifs(stat);
    std::string out;
    std::getline(ifs, out);

    const auto start = out.find('(') + 1;
    auto end = out.find(')');
    this->executable = out.substr(start, end - start);

    // remove pid and executable from string
    end += 2;
    out = out.substr(end, out.size() - end);
    const auto proc = util::str_split(out, " ");
    state = proc[0][0];
    ppid = std::stoi(proc[1]);
    tty_nr = std::stoi(proc[4]);
    minor_dev = minor(std::stoi(proc[4]));
    pty_path = fmt::format("/dev/pts/{}", minor_dev);
}
