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
#include <filesystem>
#include <xcb/xcb.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/base64.h>
#include <fmt/format.h>
#include <zmq.hpp>
#include <openssl/evp.h>
#include <iostream>

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

auto util::get_socket_path() -> std::string
{
    return fmt::format("{}/ueberzug_{}.sock", fs::temp_directory_path().string(), os::getenv("USER").value());
}

void util::send_tcp_message(std::string_view msg)
{
    auto endpoint = fmt::format("ipc://{}", get_socket_path());
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

auto util::base64_encode(const uint8_t input[], size_t length) -> std::string
{
    return Botan::base64_encode(input, length);
}

auto util::base64_encode_ssl(const unsigned char *input, int length) -> std::unique_ptr<char[]>
{
    const auto pl = 4*((length+2)/3);
    auto res = std::make_unique<char[]>(pl + 1);
    const auto ol = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(res.get()), input, length);
    if (pl != ol) {
        std::cerr << "Whoops, encode predicted " << pl << " but we got " << ol << "\n";
    }
    return res;
}


void util::move_cursor(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "f" << std::flush;
}
