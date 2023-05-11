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
#include <unordered_map>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/format.h>
#include <zmq.hpp>
#include <vips/vips8>

using json = nlohmann::json;
namespace fs = std::filesystem;

Application::Application(Flags& flags, const std::string& executable):
flags(flags),
s(SignalSingleton::instance())
{
    print_header();
    setup_logger();
    set_silent();
    terminal = std::make_unique<Terminal>(os::get_pid(), flags);
    canvas = Canvas::create(*terminal, flags, *logger, img_lock);
    if (!canvas) {
        logger->error("Unable to initialize canvas.");
        std::exit(1);
    }
    auto cache_path = util::get_cache_path();
    if (!fs::exists(cache_path)) {
        fs::create_directories(cache_path);
    }
    tmux::register_hooks();
    socket_thread = std::thread([&] {
        logger->info("Listenning for commands on socket {}.", util::get_socket_path());
        socket_loop();
    });
    if (flags.no_cache) {
        logger->info("Image caching is disabled.");
    }
    if (VIPS_INIT(executable.c_str())) {
        vips_error_exit(nullptr);
    }
    vips_cache_set_max(1);
}

Application::~Application()
{
    canvas->clear();
    vips_shutdown();
    if (f_stderr != nullptr) {
        std::fclose(f_stderr);
    }
    util::send_socket_message("EXIT", util::get_socket_endpoint());
    if (socket_thread.joinable()) {
        socket_thread.join();
    }
    tmux::unregister_hooks();
    fs::remove(util::get_socket_path());
}

void Application::execute(const std::string& cmd)
{
    json json_cmd;
    try {
        json_cmd = json::parse(cmd);
    } catch (const json::parse_error& e) {
        logger->error("There was an error parsing the command.");
        return;
    }
    logger->info("Command received: {}", json_cmd.dump());

    std::unique_lock lock {img_lock};
    if (json_cmd["action"] == "add") {
        if (!json_cmd["path"].is_string()) {
            logger->error("Invalid path.");
            return;
        }
        set_dimensions_from_json(json_cmd);
        image = Image::load(*terminal, *dimensions, flags, json_cmd["path"], *logger);
        if (!image) {
            logger->error("Unable to load image file.");
            return;
        }
        canvas->clear();
        canvas->init(*dimensions, image);
        canvas->draw();
    } else if (json_cmd["action"] == "remove") {
        logger->info("Removing image.");
        canvas->clear();
    } else if (json_cmd["action"] == "tmux") {
        handle_tmux_hook(json_cmd["hook"]);
    } else {
        logger->warn("Command not supported.");
    }
}

void Application::handle_tmux_hook(const std::string& hook)
{
    std::unordered_map<std::string_view, std::function<void()>> hook_fns {
        {"client-session-changed",
            [&] {
                if (tmux::is_window_focused()) {
                    canvas->show();
                }
            }},
        {"session-window-changed",
            [&] {
                if (tmux::is_window_focused()) {
                    canvas->show();
                } else {
                    canvas->hide();
                }
            }},
        {"client-detached",
            [&] {
                canvas->hide();
            }},
        {"window-layout-changed",
            [&] {
                if (tmux::is_window_focused()) {
                    canvas->hide();
                }
            }},
    };

    try {
        hook_fns.at(hook)();
    } catch (const std::out_of_range& oor) {
        logger->warn("TMUX hook not recognized");
    }
}

void Application::set_dimensions_from_json(const json& j)
{
    using std::string;
    int x, y, max_width, max_height;
    string width_key = "max_width", height_key = "max_height", scaler = "contain";
    if (j.contains("scaler")) {
        scaler = j["scaler"];
    }
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
    dimensions = std::make_unique<Dimensions>(*terminal, x, y, max_width, max_height, scaler);
}

void Application::setup_logger()
{
    std::string log_tmp = util::get_log_filename();
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    try {
        spdlog::flush_on(spdlog::level::info);
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path);

        auto main_logger = std::make_shared<spdlog::logger>("main", sink);
        auto terminal_logger = std::make_shared<spdlog::logger>("terminal", sink);
        auto x11_logger = std::make_shared<spdlog::logger>("X11", sink);

        spdlog::initialize_logger(main_logger);
        spdlog::initialize_logger(terminal_logger);
        spdlog::initialize_logger(x11_logger);

        logger = spdlog::get("main");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

void Application::command_loop()
{
    const int sleep_time = 100;
    if (!flags.no_stdin) {
        std::string cmd;
        while (std::getline(std::cin, cmd)) {
            if (s->get_stop_flag().load()) {
                break;
            }
            execute(cmd);
        }
    } else {
        while (!s->get_stop_flag().load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
    }
}

void Application::socket_loop()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::stream);
    socket.bind(util::get_socket_endpoint());
    const int data_size = 256;
    std::array<int, data_size> data;
    auto idbuf = zmq::buffer(data);
    while (true) {
        zmq::message_t request;
        zmq::message_t sockid;
        try {
            auto id_res = socket.recv(idbuf, zmq::recv_flags::none);
            auto result = socket.recv(request, zmq::recv_flags::none);
        } catch (const zmq::error_t& err) {}
        auto str_response = request.to_string();
        if (!str_response.empty()) {
            if (str_response == "EXIT") {
                break;
            }
            execute(str_response);
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
    if (!flags.silent) {
        return;
    }
    f_stderr = std::freopen("/dev/null", "w", stderr);
}

void Application::print_version()
{
    std::cout << "ueberzugpp " << ueberzugpp_VERSION_MAJOR << "." << ueberzugpp_VERSION_MINOR
        << "." << ueberzugpp_VERSION_PATCH << std::endl;
}
