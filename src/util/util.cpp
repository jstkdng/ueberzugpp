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

#include "util.hpp"
#include "process_info.hpp"
#include "os.hpp"

#include <xcb/xcb.h>
#include <memory>
#include <iostream>
#include <botan/hash.h>
#include <botan/hex.h>

struct free_delete
{
    void operator()(void* x) const { free(x); }
};

auto util::str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>
{
    auto start = 0U;
    auto end = str.find(delim);

    std::vector<std::string> res;

    while (end != std::string::npos) {
        res.push_back(str.substr(start, end - start));
        start = end + delim.length();
        end = str.find(delim, start);
    }

    return res;
}

auto util::get_parent_pids(const ProcessInfo& proc) -> std::vector<ProcessInfo>
{
    std::vector<ProcessInfo> res;
    ProcessInfo runner(proc.pid);
    while (runner.pid != 1) {
        auto pproc = ProcessInfo(runner.ppid);
        //if (pproc.tty_nr == 0) pproc.pty_path = runner.pty_path;
        res.push_back(runner);
        runner = pproc;
    }
    return res;
}

auto util::get_b2_hash(const std::string& str) -> std::string
{
    using namespace Botan;
    std::unique_ptr<HashFunction> hash (HashFunction::create("BLAKE2b"));
    hash->update(str);
    return hex_encode(hash->final());
}

auto util::get_cache_path() -> std::string
{
    return os::getenv("HOME").value() + "/.cache/ueberzug/";
}
