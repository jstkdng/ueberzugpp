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

#ifndef __FLAGS__
#define __FLAGS__

#include <string>
#include <filesystem>
#include <memory>

// singleton
class Flags
{
public:
    static auto instance() -> std::shared_ptr<Flags>
    {
        static std::shared_ptr<Flags> instance {new Flags};
        return instance;
    }

    Flags(const Flags&) = delete;
    Flags(Flags&) = delete;
    auto operator=(const Flags&) -> Flags& = delete;
    auto operator=(Flags&) -> Flags& = delete;

    bool no_stdin = false;
    bool silent = false;
    bool use_escape_codes = false;
    bool print_version = false;
    bool no_cache = false;
    bool no_opencv = false;
    std::string output;
    std::string pid_file;

    std::string cmd_id;
    std::string cmd_action;
    std::string cmd_socket;
    std::string cmd_x;
    std::string cmd_y;
    std::string cmd_max_width;
    std::string cmd_max_height;
    std::string cmd_file_path;

private:
    Flags();
    std::filesystem::path config_file;

    void read_config_file();
};

#endif
