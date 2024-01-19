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

#ifndef NAMESPACE_UTIL_H
#define NAMESPACE_UTIL_H

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <filesystem>
#include <functional>
#include <random>
#include <limits>
#include <optional>

#include "os.hpp"

class Flags;
class Process;

namespace util
{
    auto str_split(const std::string& str, const std::string& delim) -> std::vector<std::string>;
    auto get_process_tree(int pid) -> std::vector<int>;
    auto get_process_tree_v2(int pid) -> std::vector<Process>;
    auto get_b2_hash_ssl(std::string_view str) -> std::string;
    auto get_cache_path() -> std::string;
    auto get_cache_file_save_location(const std::filesystem::path &path) -> std::string;
    auto get_log_filename() -> std::string;
    auto get_socket_path(int pid = os::get_pid()) -> std::string;
    void send_socket_message(std::string_view msg, std::string_view endpoint);
    auto base64_encode(const unsigned char *input, size_t length) -> std::string;
    void base64_encode_v2(const unsigned char *input, size_t length, unsigned char *out);
    void move_cursor(int row, int col);
    void save_cursor_position();
    void restore_cursor_position();
    void benchmark(const std::function<void(void)>& func);
    void send_command(const Flags& flags);
    void clear_terminal_area(int xcoord, int ycoord, int width, int height);
    auto generate_random_string(std::size_t length) -> std::string;

    auto read_exif_rotation(const char* path) -> std::optional<std::uint16_t>;
    template<typename T>
    auto generate_random_number(T min, T max = std::numeric_limits<T>::max()) -> T
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<T> dist(min, max);
        return dist(rng);
    }
} // namespace util

#endif
