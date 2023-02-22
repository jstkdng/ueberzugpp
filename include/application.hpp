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

#ifndef __APPLICATION__
#define __APPLICATION__

#include "image.hpp"
#include "canvas.hpp"
#include "terminal.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <spdlog/spdlog.h>
#include <cstdlib>

class Application
{
public:
    Application();
    ~Application();

    auto execute(const std::string& cmd) -> void;
    auto command_loop(const std::atomic<bool>& flag) -> void;
    auto set_silent(bool silent = false) -> void;

private:
    Terminal terminal;

    std::shared_ptr<Image> image;
    std::unique_ptr<Canvas> canvas;
    std::shared_ptr<spdlog::logger> logger;

    auto setup_logger() -> void;
    std::FILE* f_stderr = nullptr;
};

#endif
