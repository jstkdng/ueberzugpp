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

#include "application.hpp"
#include "flags.hpp"
#include "tmux.hpp"

std::atomic<bool> stop_flag(false);

void got_signal(const int signal)
{
    stop_flag.store(true);
    auto logger = spdlog::get("main");
    switch (signal) {
        case SIGINT:
            logger->error("SIGINT received, exiting.");
            break;
        case SIGTERM:
            logger->error("SIGTERM received, exiting.");
            break;
        case SIGHUP:
            logger->error("SIGHUP received, exiting.");
            break;
        default:
            logger->error("UNKNOWN({}) signal received, exiting.", signal);
            break;
    }
}

int main(int argc, char *argv[])
{
    // handle signals
    struct sigaction sa;
    sa.sa_handler = got_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);

    Flags flags;
    std::vector<std::string> available_outputs ;

    CLI::App program("Display images in the terminal", "ueberzug");
    program.add_flag("-V,--version", flags.print_version, "Print version information.");
    CLI::App *layer_command = program.add_subcommand("layer", "Display images on the terminal.");
    layer_command->add_flag("-s,--silent", flags.silent, "Print stderr to /dev/null.");
    layer_command->add_flag("--use-escape-codes", flags.use_escape_codes, "Use escape codes to get terminal capabilities.")->default_val(false);
    layer_command->add_flag("--no-stdin", flags.no_stdin, "Don't listen on stdin for commands.");
    layer_command->add_option("-o,--output", flags.output, "Image output method")
        ->check(CLI::IsMember({"x11", "sixel", "kitty", "iterm2"}));
    layer_command->add_option("-p,--parser", nullptr, "**UNUSED**, only present for backwards compatibility.");
    layer_command->add_option("-l,--loader", nullptr, "**UNUSED**, only present for backwards compatibility.");

    auto tmux_command = program.add_subcommand("tmux", "Handle tmux hooks. Used internaly.");
    tmux_command->allow_extras();
    CLI11_PARSE(program, argc, argv);

    if (flags.print_version) {
        Application::print_version();
        std::exit(0);
    }

    if (!layer_command->parsed() && !tmux_command->parsed()) {
        program.exit(CLI::CallForHelp());
        std::exit(1);
    }

    if (layer_command->parsed()) {
        Application application(flags);
        application.command_loop(stop_flag);
    }

    if (tmux_command->parsed()) {
        try {
            auto positionals = tmux_command->remaining();
            tmux::handle_hook(positionals.at(0));
        } catch (const std::out_of_range& oor) {}
    }

    return 0;
}

