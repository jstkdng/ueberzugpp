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

#include <cstdio>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include "logging.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    bool silent;

    CLI::App program("Display images in the terminal", "ueberzug");
    CLI::App *layer_command = program.add_subcommand("layer", "Display images");
    layer_command->add_flag("-s,--silent", silent, "print stderr to /dev/null");
    program.require_subcommand(1);

    CLI11_PARSE(program, argc, argv);

    if (silent) {
        freopen("/dev/null", "w", stderr);
        freopen("/dev/null", "w", stdout);
    }

    Logging logger;

    /*
    std::string cmd;
    std::stringstream ss;
    json j;
    while (true) {
        std::cin >> cmd;
        if (cmd == "exit") break;
        ss << cmd;
        try {
            j = json::parse(ss.str());
            // clean stream
            ss.str(std::string());
            logger.log(j.dump());
            if (j["action"] == "add") {

            } else if (j["action"] == "remove") {

            }
        } catch (json::parse_error e) {
            continue;
        }
    }*/
    return 0;
}
