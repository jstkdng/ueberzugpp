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
#include <sys/types.h>

Process::Process(int pid):
pid(pid)
{
    struct proc_bsdshortinfo sproc;
    struct proc_bsdinfo proc;

    int st = proc_pidinfo(pid, PROC_PIDT_SHORTBSDINFO, 0, &sproc, PROC_PIDT_SHORTBSDINFO_SIZE);
    if (st == PROC_PIDT_SHORTBSDINFO_SIZE) {
        ppid = sproc.pbsi_ppid;
    }

    st = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, PROC_PIDTBSDINFO_SIZE);
    if (st == PROC_PIDTBSDINFO_SIZE) {
        tty_nr = proc.e_tdev;
        minor_dev = minor(tty_nr);
        pty_path = fmt::format("/dev/ttys{:0>3}", minor_dev);
    }
}

