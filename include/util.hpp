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
#include <functional>

#include "os.hpp"

class Flags;

namespace util
{
    auto str_split(const std::string& str, const std::string& delim) -> std::vector<std::string>;
    auto get_process_tree(int pid) -> std::vector<int>;
    auto get_b2_hash_ssl(std::string_view str) -> std::string;
    auto get_cache_path() -> std::string;
    auto get_cache_file_save_location(const std::filesystem::path &path) -> std::string;
    auto get_log_filename() -> std::string;
    auto get_socket_path(int pid = os::get_pid()) -> std::string;
    auto get_socket_endpoint(int pid = os::get_pid()) -> std::string;
    void send_socket_message(std::string_view msg, std::string_view endpoint);
    auto base64_encode(const unsigned char *input, uint64_t length) -> std::string;
    void base64_encode_v2(const unsigned char *input, uint64_t length, unsigned char *out);
    void move_cursor(int row, int col);
    void save_cursor_position();
    void restore_cursor_position();
    void benchmark(std::function<void(void)> func);
    void send_command(const Flags& flags);
    void clear_terminal_area(int xcoord, int ycoord, int width, int height);
} // namespace util

#endif
