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

#include <fmt/format.h>
#include <libproc.h>

#define minor(x)        ((int32_t)((x) & 0xffffff))

Process::Process(int pid):
pid(pid)
{
    struct proc_bsdinfo proc;

    int st = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, PROC_PIDTBSDINFO_SIZE);

    if (st == PROC_PIDTBSDINFO_SIZE) {
        ppid = proc.pbi_ppid;
        tty_nr = proc.e_tdev;
        minor_dev = minor(tty_nr);
        pty_path = fmt::format("/dev/ttys00{}", minor_dev);
    }
}

