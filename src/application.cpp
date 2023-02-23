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

#include "application.hpp"
#include "process_info.hpp"
#include "os.hpp"
#include "version.hpp"
#include "dimensions.hpp"
#include "util.hpp"
#include "flags.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <zmq.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

Application::Application(const Flags& flags):
terminal(ProcessInfo(os::get_pid()), flags),
flags(flags)
{
    print_header();
    setup_logger();
    set_silent();
    canvas = Canvas::create(terminal, *logger);
    auto cache_path = util::get_cache_path();
    if (!fs::exists(cache_path)) fs::create_directories(cache_path);
}

Application::~Application()
{
    canvas->clear();
    if (f_stderr) std::fclose(f_stderr);
}

auto Application::execute(const std::string& cmd) -> void
{
    json j;
    try {
        j = json::parse(cmd);
    } catch (const json::parse_error& e) {
        logger->error("There was an error parsing the command.");
        return;
    }
    logger->info("Command received: {}", j.dump());
    if (j["action"] == "add") {
        Dimensions dimensions(terminal, j["x"], j["y"], j["max_width"], j["max_height"]);
        image = Image::load(terminal, dimensions, j["path"], *logger);
        canvas->init(dimensions, image);
        if (!image) {
            logger->warn("Unable to load image file.");
            return;
        }
        canvas->draw();
    } else if (j["action"] == "remove") {
        logger->info("Removing image.");
        canvas->clear();
    } else {
        logger->warn("Command not supported.");
    }
}

auto Application::setup_logger() -> void
{
    std::string log_tmp = "ueberzug_" + os::getenv("USER").value() + ".log";
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    try {
        logger = spdlog::basic_logger_mt("main", log_path);
        logger->flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

auto Application::command_loop(const std::atomic<bool>& stop_flag) -> void
{
    if (flags.force_tcp) {
        logger->info("Listenning on port {}.", flags.tcp_port);
        tcp_loop(stop_flag);
    } else {
        std::string cmd;
        while (std::getline(std::cin, cmd)) {
            if (stop_flag.load()) break;
            execute(cmd);
        }
    }
}

auto Application::tcp_loop(const std::atomic<bool>& stop_flag) -> void
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_STREAM);
    socket.bind("tcp://127.0.0.1:" + std::to_string(flags.tcp_port));
    int data[256];
    auto idbuf = zmq::buffer(data);
    while (true) {
        zmq::message_t request, id;
        zmq::recv_result_t result;
        try {
            auto id_res = socket.recv(idbuf, zmq::recv_flags::none);
            result = socket.recv(request, zmq::recv_flags::none);
        } catch (const zmq::error_t& err) {}
        if (stop_flag.load()) break;
        if (result.has_value()) {
            auto str_response = request.to_string();
            if (!str_response.empty()) execute(str_response);
        }
    }
    socket.close();
    context.close();
}

auto Application::print_header() -> void
{
    std::string log_tmp = "ueberzug_" + os::getenv("USER").value() + ".log";
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    std::ofstream ofs(log_path, std::ios::out | std::ios::app);
    ofs << " _    _      _                                           \n"
        << "| |  | |    | |                                _     _   \n"
        << "| |  | | ___| |__   ___ _ __ _____   _  __ _ _| |_ _| |_ \n"
        << "| |  | |/ _ \\ '_ \\ / _ \\ '__|_  / | | |/ _` |_   _|_   _|\n"
        << "| |__| |  __/ |_) |  __/ |   / /| |_| | (_| | |_|   |_|  \n"
        << " \\____/ \\___|_.__/ \\___|_|  /___|\\__,_|\\__, |            \n"
        << "                                        __/ |            \n"
        << "                                       |___/     "
        << "v" << ueberzugpp_VERSION_MAJOR << "." << ueberzugpp_VERSION_MINOR
        << "." << ueberzugpp_VERSION_PATCH << std::endl;
    ofs.close();
}

auto Application::set_silent() -> void
{
    if (!flags.silent) return;
    f_stderr = freopen("/dev/null", "w", stderr);
}
