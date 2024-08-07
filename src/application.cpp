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
#include "image.hpp"
#include "tmux.hpp"
#include "util.hpp"
#include "util/socket.hpp"
#include "version.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <vips/vips8>

using njson = nlohmann::json;
namespace fs = std::filesystem;

Application::Application(const char *executable)
{
    flags = Flags::instance();
    print_header();
    setup_logger();
    set_silent();
    terminal = std::make_unique<Terminal>();
    if (flags->no_stdin) {
        daemonize();
    }
    canvas = Canvas::create();
    const auto cache_path = util::get_cache_path();
    if (!fs::exists(cache_path) && !flags->no_cache) {
        fs::create_directories(cache_path);
    }
    tmux::register_hooks();
    socket_thread = std::thread([this] {
        const auto sock_path = util::get_socket_path();
        logger->info("Listening for commands on socket {}", sock_path);
        socket_loop();
    });
    if (flags->no_cache) {
        logger->info("Image caching is disabled");
    }
    if (VIPS_INIT(executable)) {
        vips_error_exit(nullptr);
    }
    vips_cache_set_max(1);
}

Application::~Application()
{
    if (socket_thread.joinable()) {
        socket_thread.join();
    }
    logger->info("Exiting ueberzugpp");
    canvas.reset();
    vips_shutdown();
    tmux::unregister_hooks();
    fs::remove(util::get_socket_path());
}

void Application::execute(const std::string_view cmd)
{
    if (!canvas) {
        return;
    }
    njson json;
    try {
        json = njson::parse(cmd);
    } catch (const njson::parse_error &e) {
        logger->error("Command received is not valid json");
        return;
    }
    const auto json_str = json.dump();
    logger->info("Command received: {}", json_str);

    const std::string &action = json.at("action");
    if (action == "tmux") {
        const std::string &hook = json.at("hook");
        handle_tmux_hook(hook);
        return;
    }

    const std::string &identifier = json.at("identifier");
    if (action == "add") {
        if (!json.at("path").is_string()) {
            logger->error("Path received is not valid");
            return;
        }
        auto image = Image::load(json, terminal.get());
        if (!image) {
            logger->error("Unable to load image file");
            return;
        }
        canvas->add_image(identifier, std::move(image));
        init_visibility();
    } else if (action == "remove") {
        canvas->remove_image(identifier);
    } else {
        logger->warn("Command not supported");
    }
}

void Application::handle_tmux_hook(const std::string_view hook)
{
    const std::unordered_map<std::string_view, std::function<void()>> hook_fns{
        {"client-session-changed",
         [this] {
             if (tmux::is_window_focused()) {
                 canvas->show();
             } else {
                 canvas->hide();
             }
         }},
        {"session-window-changed",
         [this] {
             if (tmux::is_window_focused()) {
                 canvas->show();
             } else {
                 canvas->hide();
             }
         }},
        {"client-detached", [this] { canvas->hide(); }},
        {"window-layout-changed",
         [this] {
             if (tmux::is_window_focused()) {
                 canvas->hide();
             }
         }},
    };

    try {
        hook_fns.at(hook)();
    } catch (const std::out_of_range &oor) {
        logger->warn("TMUX hook not recognized");
    }
}

void Application::setup_logger()
{
    const auto log_tmp = util::get_log_filename();
    const auto log_path = fs::temp_directory_path() / log_tmp;
    try {
        spdlog::flush_on(spdlog::level::debug);
        const auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path);

        const auto main_logger = std::make_shared<spdlog::logger>("main", sink);
        const auto terminal_logger = std::make_shared<spdlog::logger>("terminal", sink);
        const auto cv_logger = std::make_shared<spdlog::logger>("opencv", sink);
        const auto vips_logger = std::make_shared<spdlog::logger>("vips", sink);
        const auto x11_logger = std::make_shared<spdlog::logger>("X11", sink);
        const auto sixel_logger = std::make_shared<spdlog::logger>("sixel", sink);
        const auto kitty_logger = std::make_shared<spdlog::logger>("kitty", sink);
        const auto iterm2_logger = std::make_shared<spdlog::logger>("iterm2", sink);
        const auto chafa_logger = std::make_shared<spdlog::logger>("chafa", sink);
        const auto wayland_logger = std::make_shared<spdlog::logger>("wayland", sink);
        const auto opengl_logger = std::make_shared<spdlog::logger>("opengl", sink);

        spdlog::initialize_logger(main_logger);
        spdlog::initialize_logger(terminal_logger);
        spdlog::initialize_logger(cv_logger);
        spdlog::initialize_logger(vips_logger);
        spdlog::initialize_logger(x11_logger);
        spdlog::initialize_logger(sixel_logger);
        spdlog::initialize_logger(kitty_logger);
        spdlog::initialize_logger(iterm2_logger);
        spdlog::initialize_logger(chafa_logger);
        spdlog::initialize_logger(wayland_logger);
        spdlog::initialize_logger(opengl_logger);

        logger = spdlog::get("main");
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "Log init failed: " << ex.what() << '\n';
    }
}

void Application::command_loop()
{
    if (flags->no_stdin) {
        return;
    }
    while (!stop_flag) {
        try {
            const auto in_event = os::wait_for_data_on_stdin(100);
            if (!in_event) {
                continue;
            }
            const auto cmd = os::read_data_from_stdin();
            execute(cmd);
        } catch (const std::system_error &err) {
            stop_flag = true;
            break;
        }
    }
}

void Application::socket_loop()
{
    UnixSocket socket;
    socket.bind_to_endpoint(util::get_socket_path());

    const int waitms = 100;
    int conn = -1;
    while (!stop_flag) {
        try {
            conn = socket.wait_for_connections(waitms);
        } catch (const std::system_error &err) {
            stop_flag = true;
            break;
        }

        if (conn == -1) {
            continue;
        }

        const auto data = socket.read_data_from_connection(conn);
        for (const auto &cmd : data) {
            if (cmd == "EXIT") {
                stop_flag = true;
                return;
            }
            execute(cmd);
        }
    }
}

void Application::init_visibility() {
        if (tmux::is_used() && !tmux::is_window_focused()) {
          canvas->hide();
          return;
        }
        canvas->show();
}

void Application::print_header()
{
    const auto log_tmp = util::get_log_filename();
    const auto log_path = fs::temp_directory_path() / log_tmp;
    const auto art = fmt::format(R"(
 _   _      _
| | | |    | |                                _     _
| | | | ___| |__   ___ _ __ _____   _  __ _ _| |_ _| |_
| | | |/ _ \ '_ \ / _ \ '__|_  / | | |/ _` |_   _|_   _|
| |_| |  __/ |_) |  __/ |   / /| |_| | (_| | |_|   |_|
 \___/ \___|_.__/ \___|_|  /___|\__,_|\__, |
                                       __/ |
                                      |___/     v{}.{}.{})",
                                 ueberzugpp_VERSION_MAJOR, ueberzugpp_VERSION_MINOR, ueberzugpp_VERSION_PATCH);
    std::ofstream ofs(log_path, std::ios::out | std::ios::app);
    ofs << art << '\n' << std::flush;
}

void Application::set_silent()
{
    if (!flags->silent) {
        return;
    }
    f_stderr.reset(std::freopen("/dev/null", "w", stderr));
}

void Application::print_version()
{
    const auto ver_str = fmt::format("ueberzugpp {}.{}.{}", ueberzugpp_VERSION_MAJOR, ueberzugpp_VERSION_MINOR,
                                     ueberzugpp_VERSION_PATCH);
    std::cout << ver_str << '\n';
}

void Application::daemonize()
{
    os::daemonize();
    std::ofstream ofs(flags->pid_file);
    ofs << os::get_pid() << std::flush;
}
