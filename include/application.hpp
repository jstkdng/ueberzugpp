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

#ifndef __APPLICATION__
#define __APPLICATION__

#include "image.hpp"
#include "canvas.hpp"
#include "terminal.hpp"
#include "flags.hpp"
#include "dimensions.hpp"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <string_view>

class Application
{
public:
    explicit Application(std::string_view executable);
    ~Application();

    void execute(const std::string& cmd);
    void command_loop();
    void handle_tmux_hook(std::string_view hook);

    static void print_version();
    static void print_header();
    static void daemonize(const std::string& pid_file);
    static auto get_stop_flag() -> std::shared_ptr<std::atomic<bool>>;
    static const pid_t parent_pid_;

private:
    std::unique_ptr<Terminal> terminal;
    std::unique_ptr<Dimensions> dimensions;
    std::unique_ptr<Canvas> canvas;

    std::shared_ptr<Image> image;
    std::shared_ptr<Flags> flags;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<std::atomic<bool>> stop_flag;

    std::FILE* f_stderr = nullptr;
    std::thread socket_thread;
    std::mutex img_lock;

    void setup_logger();
    void set_silent();
    void socket_loop();
    void set_dimensions_from_json(const nlohmann::json& json);
};

#endif
