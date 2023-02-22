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
#include "canvas/x11/x11.hpp"

#include <spdlog/spdlog.h>

auto Canvas::create(const Terminal& terminal, spdlog::logger& logger) -> std::unique_ptr<Canvas>
{
    if (terminal.supports_sixel()) {
        logger.info("Terminal is {}, using sixel output.", terminal.name);
        return std::make_unique<SixelCanvas>();
    }
    logger.info("Terminal is {}, using X11 output.", terminal.name);
    return std::make_unique<X11Canvas>(terminal);
}
