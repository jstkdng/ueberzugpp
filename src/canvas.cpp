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
#include "canvas/chafa.hpp"
#include "canvas/kitty/kitty.hpp"
#include "canvas/iterm2/iterm2.hpp"
#ifdef ENABLE_X11
#   include "canvas/x11/x11.hpp"
#endif
#include "flags.hpp"
#include "os.hpp"

auto Canvas::create() -> std::unique_ptr<Canvas>
{
    const auto flags = Flags::instance();
    const auto logger = spdlog::get("main");

#ifdef ENABLE_X11
    auto xdg_session = os::getenv("XDG_SESSION_TYPE").value_or("");
    if (xdg_session == "wayland") {
        if (flags->output == "x11") {
            logger->warn("Forcing X11 output");
        } else {
            logger->info("Wayland detected, X11 output will not be used"
                " unless forced to");
        }
    }
    if (flags->output == "x11") {
        return std::make_unique<X11Canvas>();
    }
#else
    logger->debug("X11 support not compiled in the binary");
#endif
    if (flags->output == "kitty") {
        return std::make_unique<KittyCanvas>();
    }
    if (flags->output == "iterm2") {
        return std::make_unique<Iterm2Canvas>();
    }
    if (flags->output == "sixel") {
        return std::make_unique<SixelCanvas>();
    }
    flags->output = "chafa";
    return std::make_unique<ChafaCanvas>();
}
