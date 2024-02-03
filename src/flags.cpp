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

#include <fmt/format.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// read configuration file
Flags::Flags()
{
    const auto home = os::getenv("HOME").value_or(fs::temp_directory_path());
    const auto config_home = os::getenv("XDG_CONFIG_HOME").value_or(fmt::format("{}/.config", home));
    config_file = fmt::format("{}/ueberzugpp/config.json", config_home);
    if (fs::exists(config_file)) {
        read_config_file();
    }
}

void Flags::read_config_file()
{
    std::ifstream ifs(config_file);
    const auto data = json::parse(ifs);
    if (!data.contains("layer")) {
        return;
    }
    const auto &layer = data.at("layer");
    silent = layer.value("silent", false);
    output = layer.value("output", "");
    no_cache = layer.value("no-cache", false);
    no_opencv = layer.value("no-opencv", false);
    use_opengl = layer.value("opengl", false);
}
