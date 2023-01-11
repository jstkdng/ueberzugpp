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
#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>
#include <thread>
#include <atomic>

#include "display.hpp"
#include "logging.hpp"
#include "os.hpp"
#include "tmux.hpp"

using json = nlohmann::json;

std::atomic<bool> quit(false);

void got_signal(int)
{
    quit.store(true);
}

int main(int argc, char *argv[])
{
    // handle SIGINT and SIGTERM
    /*
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    */

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

    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }

    Logging logger;
    Display display(logger);
    display.create_window();

    std::thread t1 = display.spawn_event_handler();

    auto windows = display.get_server_window_ids();
    for (auto window: windows) {
        std::cout << window << " ";
    }
    std::cout << std::endl;

    std::string cmd;
    json j;
    while (std::getline(std::cin, cmd)) {
        try {
            j = json::parse(cmd);
            logger.log(j.dump());
            if (j["action"] == "add") {
                display.load_image(j["path"]);
            } else if (j["action"] == "remove") {
                display.destroy_image();  
            }
        } catch (json::parse_error e) {
            continue;
        }
        if (quit.load()) break;
    }
    // TODO: send exit event to thread
    t1.join();
    vips_shutdown();
    return 0;
}
