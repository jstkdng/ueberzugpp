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

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>
#include <atomic>
#include <csignal>
#include <cstring>
#include <vips/vips8>
#include "tmux.hpp"

#include "application.hpp"
#include "logging.hpp"

std::atomic<bool> quit(false);

void got_signal(int)
{
    quit.store(true);
}

int main(int argc, char *argv[])
{
    // handle SIGINT and SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    bool silent = false;
    CLI::App program("Display images in the terminal", "ueberzug");
    CLI::App *layer_command = program.add_subcommand("layer", "Display images");
    layer_command->add_flag("-s,--silent", silent, "print stderr to /dev/null");
    program.require_subcommand(1);

    CLI11_PARSE(program, argc, argv);

    logger.set_silent(silent);

    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
    }
    vips_concurrency_set(1);

    Application application;

    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (quit.load()) break;
        application.execute(cmd);
    }

    vips_shutdown();
    return 0;
}

