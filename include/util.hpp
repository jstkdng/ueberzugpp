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

#ifndef __NAMESPACE_UTIL__
#define __NAMESPACE_UTIL__

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <filesystem>

namespace util
{
    auto str_split(const std::string& str, const std::string& delim) -> std::vector<std::string>;
    auto get_process_tree(int pid) -> std::vector<int>;
    auto get_b2_hash_ssl(const std::string& str) -> std::string;
    auto get_cache_path() -> std::string;
    auto get_cache_file_save_location(const std::filesystem::path &path) -> std::string;
    auto get_log_filename() -> std::string;
    void send_tcp_message(std::string_view msg);
    auto get_socket_path() -> std::string;
    auto base64_encode_ssl(const unsigned char *input, int length) -> std::unique_ptr<unsigned char[]>;
    void move_cursor(int row, int col);
}

#endif
