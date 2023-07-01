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

#include "sixel.hpp"
#include "image.hpp"
#include "stdout.hpp"

SixelCanvas::SixelCanvas()
{
    logger = spdlog::get("sixel");
    logger->info("Canvas created");
}

void SixelCanvas::add_image(const std::string& identifier, std::unique_ptr<Image> new_image)
{
    remove_image(identifier);
    images.insert({identifier, std::make_unique<SixelStdout>(std::move(new_image), stdout_mutex)});
    images.at(identifier)->draw();
}

void SixelCanvas::remove_image(const std::string& identifier)
{
    images.erase(identifier);
}
