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

#include "canvas/chafa.hpp"
#include "canvas/iterm2/iterm2.hpp"
#include "canvas/kitty/kitty.hpp"
#include "canvas/sixel.hpp"
#include "canvas/stdout.hpp"
#ifdef ENABLE_X11
#  include "canvas/x11/x11.hpp"
#endif
#ifdef ENABLE_WAYLAND
#  include "canvas/wayland/wayland.hpp"
#endif
#include "flags.hpp"
#include <fmt/format.h>

auto Canvas::create() -> std::unique_ptr<Canvas>
{
    const auto flags = Flags::instance();
    const auto logger = spdlog::get("main");

#ifdef ENABLE_WAYLAND
    if (flags->output == "wayland") {
        return std::make_unique<WaylandCanvas>();
    }
#else
    logger->debug("Wayland support not compiled in the binary");
#endif
#ifdef ENABLE_X11
    if (flags->output == "x11") {
        return std::make_unique<X11Canvas>();
    }
#else
    logger->debug("X11 support not compiled in the binary");
#endif

    if (flags->output == "kitty") {
        return std::make_unique<StdoutCanvas<Kitty>>(flags->output);
    }
    if (flags->output == "iterm2") {
        return std::make_unique<StdoutCanvas<Iterm2>>(flags->output);
    }
    if (flags->output == "sixel") {
        return std::make_unique<StdoutCanvas<Sixel>>(flags->output);
    }
    if (flags->output == "chafa") {
        return std::make_unique<StdoutCanvas<Chafa>>(flags->output);
    }
    throw std::runtime_error(fmt::format("output backend not supported (backend is {})", flags->output));
}
