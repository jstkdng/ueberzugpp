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

#include "logging.hpp"
#include "os.hpp"

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

Logging::Logging()
{
    std::string log_tmp = "ueberzug_" + os::getenv("USER").value() + ".log";
    fs::path log_path = fs::temp_directory_path() / log_tmp;
    this->logfile = std::ofstream(log_path, std::ios_base::app);
}

void Logging::set_silent(bool silent)
{
    if (silent) fp = freopen("/dev/null", "w", stderr);
}

Logging::~Logging()
{
    if (fp) fclose(fp);
}

