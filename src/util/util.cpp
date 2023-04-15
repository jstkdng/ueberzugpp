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

#include <memory>
#include <regex>
#include <fmt/format.h>
#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/evp.h>
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#  define EVP_MD_CTX_new   EVP_MD_CTX_create
#  define EVP_MD_CTX_free  EVP_MD_CTX_destroy
#endif

namespace fs = std::filesystem;

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

auto util::get_cache_path() -> std::string
{
    return fmt::format("{}/.cache/ueberzugpp/", os::getenv("HOME").value());
}

auto util::get_log_filename() -> std::string
{
    return fmt::format("ueberzugpp-{}.log", os::getenv("USER").value());
}

auto util::get_socket_endpoint(int pid) -> std::string
{
    return fmt::format("ipc://{}", get_socket_path(pid));
}

auto util::get_socket_path(int pid) -> std::string
{
    return fmt::format("{}/ueberzugpp-{}.socket", fs::temp_directory_path().string(), pid);
}

void util::send_socket_message(const std::string& msg, const std::string& endpoint)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::stream);
    socket.set(zmq::sockopt::linger, 2);
    socket.connect(endpoint);
    auto id_sock = socket.get(zmq::sockopt::routing_id);
    zmq::message_t id_req(id_sock), msg_req(msg);
    socket.send(id_req, zmq::send_flags::sndmore);
    socket.send(msg_req, zmq::send_flags::none);
    socket.close();
    context.close();
}

auto util::base64_encode_ssl(const unsigned char *input, int length) -> std::unique_ptr<unsigned char[]>
{
    const auto pl = 4*((length+2)/3);
    auto res = std::make_unique<unsigned char[]>(pl + 1);
    const auto ol = EVP_EncodeBlock(res.get(), input, length);
    if (pl != ol) return nullptr;
    return res;
}

auto util::get_b2_hash_ssl(const std::string& str) -> std::string
{
    std::stringstream ss;
    auto mdctx = std::unique_ptr<EVP_MD_CTX, evp_md_ctx_deleter> {
        EVP_MD_CTX_new()
    };
    auto evp = EVP_blake2b512();
    auto digest = std::make_unique<unsigned char[]>(EVP_MD_size(evp));

    EVP_DigestInit_ex(mdctx.get(), evp, nullptr);
    EVP_DigestUpdate(mdctx.get(), str.c_str(), str.size());
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(mdctx.get(), digest.get(), &digest_len);

    for (int i = 0; i < digest_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return std::move(ss).str();
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
    return fmt::format("{}{}{}", get_cache_path(), get_b2_hash_ssl(path), path.extension().string());
}

void util::benchmark(std::function<void(void)> func)
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    auto t1 = high_resolution_clock::now();
    func();
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;

    std::cout << ms_double.count() << "ms\n";
}
