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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "canvas.hpp"
#include "flags.hpp"
#include "os.hpp"
#include "terminal.hpp"
#include "util/ptr.hpp"

#include <atomic>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

#include <spdlog/spdlog.h>

class Application
{
  public:
    explicit Application(const char *executable);
    ~Application();

    void execute(std::string_view cmd);
    void command_loop();
    void handle_tmux_hook(std::string_view hook);

    inline static std::atomic<bool> stop_flag = false; // NOLINT
    inline static const int parent_pid = os::get_ppid();

    static void print_version();
    static void print_header();

  private:
    std::unique_ptr<Terminal> terminal;
    std::unique_ptr<Canvas> canvas;

    std::shared_ptr<Flags> flags;
    std::shared_ptr<spdlog::logger> logger;

    cn_unique_ptr<std::FILE, std::fclose> f_stderr;
    std::thread socket_thread;

    void setup_logger();
    void set_silent();
    void socket_loop();
    void daemonize();
    void init_visibility();
};

#endif
