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

#include <nlohmann/json.hpp>
#include <vips/vips8>
#include <string>
#include <thread>
#include <chrono>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include "display.hpp"
#include "logging.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }

    bool silent = false;

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
    Display display(logger);
    display.create_window();

    std::thread t1 = display.spawn_event_handler();

    std::string cmd;
    json j;
    while (std::getline(std::cin, cmd)) {
        try {
            j = json::parse(cmd);
            if (j["action"] == "add") {
                display.load_image(j["path"]);
            } else if (j["action"] == "remove") {

            }
        } catch (json::parse_error e) {
            continue;
        }
    }
    t1.join();
    vips_shutdown();
    return 0;
}
