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

#include "canvas.hpp"
#include "canvas/sixel.hpp"
#include "canvas/kitty.hpp"
#include "canvas/x11/x11.hpp"
#include "os.hpp"

#include <spdlog/spdlog.h>

auto Canvas::create(const Terminal& terminal, Flags& flags,
        spdlog::logger& logger, std::mutex& img_lock) -> std::unique_ptr<Canvas>
{
    if (flags.output.empty()) flags.output = "x11";

    logger.info("TERM=\"{}\", TERM_PROGRAM=\"{}\", OUTPUT=\"{}\"",
            terminal.term , terminal.term_program, flags.output);
    if (flags.output == "sixel") {
        return std::make_unique<SixelCanvas>(img_lock);
    }
    if (flags.output == "kitty") {
        return std::make_unique<KittyCanvas>();
    }
    return std::make_unique<X11Canvas>(img_lock);
}
