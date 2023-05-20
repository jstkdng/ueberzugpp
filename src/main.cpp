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
#include <csignal>
#include <fmt/format.h>
#include <spdlog/cfg/env.h>

#include "application.hpp"
#include "flags.hpp"
#include "tmux.hpp"
#include "util.hpp"

void signal_handler(const int signal)
{
    auto& flag = Application::stop_flag_;
    flag.store(true);

    auto logger = spdlog::get("main");
    if (!logger) {
        return;
    }
    switch (signal) {
        case SIGINT:
            logger->error("SIGINT received, exiting.");
            break;
        case SIGTERM:
            logger->error("SIGTERM received, exiting.");
            break;
        default:
            logger->error("UNKNOWN({}) signal received, exiting.", signal);
            break;
    }
}

auto main(int argc, char *argv[]) -> int
{
    // handle signals
    struct sigaction sga;
    sga.sa_handler = signal_handler;
    sigemptyset(&sga.sa_mask);
    sga.sa_flags = 0;
    sigaction(SIGINT, &sga, nullptr);
    sigaction(SIGTERM, &sga, nullptr);
    sigaction(SIGHUP, nullptr, nullptr);
    sigaction(SIGCHLD, nullptr, nullptr);

    spdlog::cfg::load_env_levels();
    auto flags = Flags::instance();

    CLI::App program("Display images in the terminal", "ueberzug");
    program.add_flag("-V,--version", flags->print_version, "Print version information.");

    auto *layer_command = program.add_subcommand("layer", "Display images on the terminal.");
    layer_command->add_flag("-s,--silent", flags->silent, "Print stderr to /dev/null.");
    layer_command->add_flag("--use-escape-codes", flags->use_escape_codes, "Use escape codes to get terminal capabilities.")->default_val(false);
    layer_command->add_option("--pid-file", flags->pid_file, "Output file where to write the daemon PID.");
    layer_command->add_flag("--no-stdin", flags->no_stdin, "Do not listen on stdin for commands.")->needs("--pid-file");
    layer_command->add_flag("--no-cache", flags->no_cache, "Disable caching of resized images.");
    layer_command->add_flag("--no-opencv", flags->no_opencv, "Do not use OpenCV, use Libvips instead.");
    layer_command->add_option("-o,--output", flags->output, "Image output method")
        ->check(CLI::IsMember({"x11", "sixel", "kitty", "iterm2", "chafa"}));
    layer_command->add_option("-p,--parser", nullptr, "**UNUSED**, only present for backwards compatibility.");
    layer_command->add_option("-l,--loader", nullptr, "**UNUSED**, only present for backwards compatibility.");

    auto *cmd_comand = program.add_subcommand("cmd", "Send a command to a running ueberzugpp instance.");
    cmd_comand->add_option("-s,--socket", flags->cmd_socket, "UNIX socket of running instance");
    cmd_comand->add_option("-i,--identifier", flags->cmd_id, "Preview identifier");
    cmd_comand->add_option("-a,--action", flags->cmd_action, "Action to send");
    cmd_comand->add_option("-f,--file", flags->cmd_file_path, "Path of image file");
    cmd_comand->add_option("-x,--xpos", flags->cmd_x, "X position of preview");
    cmd_comand->add_option("-y,--ypos", flags->cmd_y, "Y position of preview");
    cmd_comand->add_option("--max-width", flags->cmd_max_width, "Max width of preview");
    cmd_comand->add_option("--max-height", flags->cmd_max_height, "Max height of preview");

    auto *tmux_command = program.add_subcommand("tmux", "Handle tmux hooks. Used internaly.");
    tmux_command->allow_extras();

    auto *query_win_command = program.add_subcommand("query_windows", "**UNUSED**, only present for backwards compatibility.");
    query_win_command->allow_extras();

    CLI11_PARSE(program, argc, argv);

    if (query_win_command->parsed()) {
        std::exit(0);
    }

    if (flags->print_version) {
        Application::print_version();
        std::exit(0);
    }

    if (!layer_command->parsed() && !tmux_command->parsed() && !cmd_comand->parsed()) {
        program.exit(CLI::CallForHelp());
        std::exit(1);
    }

    if (layer_command->parsed()) {
        Application application(argv[0]);
        application.command_loop();
    }

    if (tmux_command->parsed()) {
        try {
            const auto positionals = tmux_command->remaining();
            tmux::handle_hook(positionals.at(0), std::stoi(positionals.at(1)));
        } catch (const std::out_of_range& oor) {}
    }

    if (cmd_comand->parsed()) {
        util::send_command(*flags);
    }

    return 0;
}

