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
#include "flags.hpp"

#include <memory>
#include <regex>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>

#include <fmt/format.h>
#include <zmq.hpp>
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

auto util::get_socket_endpoint(int pid) -> std::string
{
    return fmt::format("ipc://{}", get_socket_path(pid));
}

auto util::get_socket_path(int pid) -> std::string
{
    return fmt::format("{}/ueberzugpp-{}.socket", fs::temp_directory_path().string(), pid);
}

void util::send_socket_message(const std::string_view msg, const std::string_view endpoint)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::stream);
    socket.set(zmq::sockopt::linger, 2);
    socket.connect(endpoint.data());
    const auto id_sock = socket.get(zmq::sockopt::routing_id);
    zmq::message_t id_req(id_sock);
    zmq::message_t msg_req(msg);
    socket.send(id_req, zmq::send_flags::sndmore);
    socket.send(msg_req, zmq::send_flags::none);
    socket.close();
    context.close();
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

    auto ti1 = high_resolution_clock::now();
    func();
    auto ti2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = ti2 - ti1;

    std::cout << ms_double.count() << "ms\n";
}

void util::send_command(const Flags& flags)
{
    if (flags.cmd_action == "exit") {
        auto endpoint = fmt::format("ipc://{}", flags.cmd_socket);
        util::send_socket_message("EXIT", endpoint);
        return;
    }
    if (flags.cmd_action == "remove") {
        auto json = fmt::format(R"({{"action":"remove","identifier":"{}"}})", flags.cmd_id);
        auto endpoint = fmt::format("ipc://{}", flags.cmd_socket);
        util::send_socket_message(json, endpoint);
        return;
    }
    auto xcoord = std::stoi(flags.cmd_x);
    auto ycoord = std::stoi(flags.cmd_y);
    auto max_width = std::stoi(flags.cmd_max_width);
    auto max_height = std::stoi(flags.cmd_max_height);

    auto json = fmt::format(R"({{"action":"{}","identifier":"{}","max_width":{},"max_height":{},"x":{},"y":{},"path":"{}"}})",
            flags.cmd_action, flags.cmd_id, max_width, max_height, xcoord, ycoord, flags.cmd_file_path);
    auto endpoint = fmt::format("ipc://{}", flags.cmd_socket);
    util::send_socket_message(json, endpoint);
}

void util::clear_terminal_area(int xcoord, int ycoord, int width, int height)
{
    save_cursor_position();
    auto line_clear = std::string(width, ' ');
    for (int i = ycoord; i <= height + 2; ++i) {
        util::move_cursor(i, xcoord);
        std::cout << line_clear;
    }
    std::cout << std::flush;
    restore_cursor_position();
}
