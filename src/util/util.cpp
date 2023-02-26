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
#include "process.hpp"
#include "os.hpp"

#include <memory>
#include <regex>
#include <xcb/xcb.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <fmt/format.h>

auto util::str_split(const std::string& str, const std::string& delim) -> std::vector<std::string>
{
    std::regex re {delim};
    std::sregex_token_iterator
        first {str.begin(), str.end(), re, -1}, last;
    return {first, last};
}

auto util::get_process_tree(int pid) -> std::vector<int>
{
    std::vector<int> res;
    Process runner(pid);
    while (runner.pid != 1) {
        auto pproc = Process(runner.ppid);
        res.push_back(runner.pid);
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
    return fmt::format("{}/.cache/ueberzug/", os::getenv("HOME").value());
}

auto util::get_log_filename() -> std::string
{
    return fmt::format("ueberzug_{}.log", os::getenv("USER").value());
}
