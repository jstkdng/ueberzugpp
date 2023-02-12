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

#include "process_info.hpp"
#include "util.hpp"
#include "tmux.hpp"
#include "os.hpp"

#include <fstream>
#include <string>
#include <iostream>
#include <sys/sysmacros.h>

ProcessInfo::ProcessInfo(int pid):
pid(pid)
{
    std::string stat = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream is(stat);
    std::string out;
    std::getline(is, out);

    int start = out.find('(') + 1, end = out.find(')');
    this->executable = out.substr(start, end - start);

    // remove pid and executable from string
    end += 2;
    out = out.substr(end, out.size() - end);
    auto proc = util::str_split(out, " ");
    this->state = proc[0][0];
    this->ppid = std::stoi(proc[1]);
    this->tty_nr = std::stoi(proc[4]);
    this->minor_dev = minor(this->tty_nr);
    this->pty_path = "/dev/pts/" + std::to_string(this->minor_dev);
    is.close();
}

ProcessInfo::~ProcessInfo()
{}
