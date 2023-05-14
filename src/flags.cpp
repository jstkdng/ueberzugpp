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

#include "flags.hpp"
#include "os.hpp"

#include <iostream>
#include <fmt/format.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// read configuration file
Flags::Flags()
{
    auto home = os::getenv("HOME").value_or(fs::temp_directory_path());
    auto config_home = os::getenv("XDG_CONFIG_HOME").value_or(
            fmt::format("{}/.config", home));
    config_file = fmt::format("{}/ueberzugpp/config.json", config_home);
    if (fs::exists(config_file)) {
        read_config_file();
    }
}

void Flags::read_config_file()
{
    std::ifstream ifs(config_file);
    try {
        json data = json::parse(ifs);
        if (!data.contains("layer")) {
            return;
        }
        data = data["layer"];
        if (data.contains("silent")) {
            silent = data["silent"];
        }
        if (data.contains("output")) {
            output = data["output"];
        }
        if (data.contains("no-cache")) {
            no_cache = data["no-cache"];
        }
        if (data.contains("no-opencv")) {
            no_opencv = data["no-opencv"];
        }
    } catch (const json::parse_error& e) {
        std::cerr << "Could not parse config file." << std::endl;
        std::exit(1);
    }
}
