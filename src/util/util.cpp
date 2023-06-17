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
#include "util/ptr.hpp"
#include "util/socket.hpp"
#include "flags.hpp"

#include <memory>
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>
#include <random>

#include <fmt/format.h>
#include <openssl/evp.h>
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#   define EVP_MD_CTX_new   EVP_MD_CTX_create
#   define EVP_MD_CTX_free  EVP_MD_CTX_destroy
#endif
#ifdef ENABLE_TURBOBASE64
#   ifdef WITH_SYSTEM_TURBOBASE64
#       include <turbobase64/turbob64.h>
#   else
#       include "turbob64.h"
#   endif
#endif

namespace fs = std::filesystem;

auto util::str_split(const std::string& str, const std::string& delim) -> std::vector<std::string>
{
    const std::regex regex {delim};
    const std::sregex_token_iterator first {str.begin(), str.end(), regex, -1};
    const std::sregex_token_iterator last;
    return {first, last};
}

auto util::get_process_tree(int pid) -> std::vector<int>
{
    std::vector<int> res;
    Process runner(pid);
    while (runner.pid > 1) {
        res.push_back(runner.pid);
        runner = Process(runner.ppid);
    }
    return res;
}

auto util::get_process_tree_v2(int pid) -> std::vector<Process>
{
    std::vector<Process> tree;
    Process runner(pid);
    while (runner.pid > 1) {
        tree.push_back(runner);
        runner = Process(runner.ppid);
    }
    return tree;
}

auto util::get_cache_path() -> std::string
{
    const auto home = os::getenv("HOME").value_or(fs::temp_directory_path());
    return fmt::format("{}/.cache/ueberzugpp/", home);
}

auto util::get_log_filename() -> std::string
{
    const auto user = os::getenv("USER").value_or("NOUSER");
    return fmt::format("ueberzugpp-{}.log", user);
}

auto util::get_socket_path(int pid) -> std::string
{
    return fmt::format("{}/ueberzugpp-{}.socket", fs::temp_directory_path().string(), pid);
}

void util::send_socket_message(const std::string_view msg, const std::string_view endpoint)
{
    try {
        UnixSocket socket;
        socket.connect_to_endpoint(endpoint);
        socket.write(msg.data(), msg.size());
    } catch (const std::system_error& err) {}
}

auto util::base64_encode(const unsigned char *input, uint64_t length) -> std::string
{
    size_t bufsize = 4 * ((length+2)/3);
    auto res = std::vector<char>(bufsize + 1, 0);
    base64_encode_v2(input, length, reinterpret_cast<unsigned char*>(res.data()));
    return { res.data() };
}

void util::base64_encode_v2(const unsigned char *input, uint64_t length, unsigned char *out)
{
#ifdef ENABLE_TURBOBASE64
    tb64enc(input, length, out);
#else
    EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out), input, length);
#endif
}

auto util::get_b2_hash_ssl(const std::string_view str) -> std::string
{
    std::stringstream sstream;
    const auto mdctx = std::unique_ptr<EVP_MD_CTX, evp_md_ctx_deleter> {
        EVP_MD_CTX_new()
    };
    const auto *evp = EVP_blake2b512();
    auto digest = std::vector<unsigned char>(EVP_MD_size(evp), 0);

    EVP_DigestInit_ex(mdctx.get(), evp, nullptr);
    EVP_DigestUpdate(mdctx.get(), str.data(), str.size());
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(mdctx.get(), digest.data(), &digest_len);

    for (uint32_t i = 0; i < digest_len; ++i) {
        sstream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return sstream.str();
}

void util::move_cursor(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "f" << std::flush;
}

void util::save_cursor_position()
{
    std::cout << "\0337" << std::flush;
}

void util::restore_cursor_position()
{
    std::cout << "\0338" << std::flush;
}

auto util::get_cache_file_save_location(const fs::path &path) -> std::string
{
    return fmt::format("{}{}{}", get_cache_path(), get_b2_hash_ssl(path.string()), path.extension().string());
}

void util::benchmark(std::function<void(void)> func)
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    const auto ti1 = high_resolution_clock::now();
    func();
    const auto ti2 = high_resolution_clock::now();
    const duration<double, std::milli> ms_double = ti2 - ti1;

    std::cout << ms_double.count() << "ms\n";
}

void util::send_command(const Flags& flags)
{
    if (flags.cmd_action == "exit") {
        util::send_socket_message("EXIT", flags.cmd_socket);
        return;
    }
    if (flags.cmd_action == "remove") {
        const auto json = fmt::format(R"({{"action":"remove","identifier":"{}"}})", flags.cmd_id);
        util::send_socket_message(json, flags.cmd_socket);
        return;
    }
    const auto xcoord = std::stoi(flags.cmd_x);
    const auto ycoord = std::stoi(flags.cmd_y);
    const auto max_width = std::stoi(flags.cmd_max_width);
    const auto max_height = std::stoi(flags.cmd_max_height);

    const auto json = fmt::format(R"({{"action":"{}","identifier":"{}","max_width":{},"max_height":{},"x":{},"y":{},"path":"{}"}})",
            flags.cmd_action, flags.cmd_id, max_width, max_height, xcoord, ycoord, flags.cmd_file_path);
    util::send_socket_message(json, flags.cmd_socket);
}

void util::clear_terminal_area(int xcoord, int ycoord, int width, int height)
{
    save_cursor_position();
    const auto line_clear = std::string(width, ' ');
    for (int i = ycoord; i <= height + 2; ++i) {
        util::move_cursor(i, xcoord);
        std::cout << line_clear;
    }
    std::cout << std::flush;
    restore_cursor_position();
}

auto util::generate_random_string(std::size_t length) -> std::string
{
    constexpr auto chars = std::to_array({
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'
    });
    auto rng_dev = std::random_device();
    auto rng = std::mt19937(rng_dev());
    auto dist = std::uniform_int_distribution{{}, chars.size() - 1};
    std::string result(length, 0);
    std::generate_n(std::begin(result), length, [&] { return chars[dist(rng)]; });
    return result;
}
