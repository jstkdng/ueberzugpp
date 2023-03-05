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
#include "os.hpp"
#include "version.hpp"
#include "util.hpp"
#include "flags.hpp"
#include "tmux.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <spdlog/sinks/basic_file_sink.h>
#include <opencv2/core/ocl.hpp>
#include <fmt/format.h>
#include <zmq.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

Application::Application(const Flags& flags):
terminal(os::get_pid(), flags),
flags(flags)
{
    print_header();
    setup_logger();
    set_silent();
    canvas = Canvas::create(terminal, *logger);
    auto cache_path = util::get_cache_path();
    if (!fs::exists(cache_path)) fs::create_directories(cache_path);
    tmux::register_hooks();
    tcp_thread = std::jthread([&] {
        logger->info("Listenning on port {}.", flags.tcp_port);
        tcp_loop();
    });
    cv::ocl::Context ctx = cv::ocl::Context::getDefault();
    if (ctx.ptr()) {
        auto device = ctx.device(0);
        logger->info("OpenCL supported. Using device {}.", device.name());
    }
}

Application::~Application()
{
    canvas->clear();
    if (f_stderr) std::fclose(f_stderr);
    util::send_tcp_message("EXIT", flags.tcp_port);
    tmux::unregister_hooks();
}

void Application::execute(const std::string& cmd)
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
        set_dimensions_from_json(j);
        image = Image::load(terminal, *dimensions, j["path"], *logger);
        if (!image) {
            logger->error("Unable to load image file.");
            return;
        }
        canvas->init(*dimensions, image);
        canvas->draw();
    } else if (j["action"] == "remove") {
        logger->info("Removing image.");
        canvas->clear();
    } else if (j["action"] == "tmux") {
        handle_tmux_hook(j["hook"]);
    } else {
        logger->warn("Command not supported.");
    }
}

void Application::handle_tmux_hook(const std::string& hook)
{
    if (hook == "client-session-changed") {
        canvas->show();
    } else if (hook == "session-window-changed") {
        canvas->toggle();
    } else if (hook == "client-detached") {
        canvas->hide();
    } else if (hook == "pane-mode-changed") {
        // TODO: what to do here?
    } else {
        logger->warn("TMUX hook not recognized");
    }
}

void Application::set_dimensions_from_json(const json& j)
{
    using std::string;
    int x, y, max_width, max_height;
    string width_key = "max_width", height_key = "max_height";
    if (j.contains("width")) {
        width_key = "width";
        height_key = "height";
    }
    if (j[width_key].is_string()) {
        string w = j[width_key], h = j[height_key];
        max_width = std::stoi(w);
        max_height = std::stoi(h);
    } else {
        max_width = j[width_key];
        max_height = j[height_key];
    }
    if (j["x"].is_string()) {
        string _x = j["x"], _y = j["y"];
        x = std::stoi(_x);
        y = std::stoi(_y);
    } else {
        x = j["x"];
        y = j["y"];
    }
    dimensions = std::make_unique<Dimensions>(terminal, x, y, max_width, max_height);
}

void Application::setup_logger()
{
    std::string log_tmp = util::get_log_filename();
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    try {
        logger = spdlog::basic_logger_mt("main", log_path);
        logger->flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

void Application::command_loop(const std::atomic<bool>& stop_flag)
{
    if (!flags.no_stdin) {
        std::string cmd;
        while (std::getline(std::cin, cmd)) {
            if (stop_flag.load()) break;
            execute(cmd);
        }
    } else {
        while (!stop_flag.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void Application::tcp_loop()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::stream);
    socket.bind(fmt::format("tcp://127.0.0.1:{}", flags.tcp_port));
    int data[256];
    auto idbuf = zmq::buffer(data);
    while (true) {
        zmq::message_t request, id;
        try {
            auto id_res = socket.recv(idbuf, zmq::recv_flags::none);
            auto result = socket.recv(request, zmq::recv_flags::none);
        } catch (const zmq::error_t& err) {}
        auto str_response = request.to_string();
        if (!str_response.empty()) {
            if (str_response == "EXIT") break;
            for (const auto& cmd: util::str_split(str_response, "\n")) {
                if (!cmd.empty()) execute(cmd);
            }
        }
    }
    socket.close();
    context.close();
}

void Application::print_header()
{
    std::string log_tmp = util::get_log_filename();
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

void Application::set_silent()
{
    if (!flags.silent) return;
    f_stderr = freopen("/dev/null", "w", stderr);
}

void Application::print_version()
{
    std::cout << "ueberzugpp " << ueberzugpp_VERSION_MAJOR << "." << ueberzugpp_VERSION_MINOR
        << "." << ueberzugpp_VERSION_PATCH << std::endl;
}
