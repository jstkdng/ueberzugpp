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

#include "libvips.hpp"

#include <unordered_set>

using namespace vips;

LibvipsImage::LibvipsImage(const Terminal& terminal,
        const std::string& filename, int max_width, int max_height):
terminal(terminal),
path(filename),
max_width(max_width * terminal.font_width),
max_height(max_height * terminal.font_height)
{
    auto check = VImage::new_from_file(filename.c_str());
    VImage img;
    // at least 3 bands are required
    if (check.width() < this->max_width && check.height() < this->max_height) {
        // thumbnail not required
        img = check.colourspace(VIPS_INTERPRETATION_sRGB);
    } else {
        img = VImage::thumbnail(filename.c_str(), this->max_width - 1)
            .colourspace(VIPS_INTERPRETATION_sRGB);
    }
    if (terminal.supports_sixel()) {
        // sixel expects RGB888
        if (img.has_alpha()) {
            img = img.flatten();
        }
    } else {
        // alpha channel required
        if (!img.has_alpha()) img = img.bandjoin(255);
        // convert from RGB to BGR
        auto bands = img.bandsplit();
        auto tmp = bands[0];
        bands[0] = bands[2];
        bands[2] = tmp;
        img = VImage::bandjoin(bands);
    }

    image = img;
    _size = VIPS_IMAGE_SIZEOF_IMAGE(image.get_image());
    _data.reset(static_cast<unsigned char*>(image.write_to_memory(&(_size))));
}

auto LibvipsImage::width() const -> int
{
    return image.width();
}

auto LibvipsImage::height() const -> int
{
    return image.height();
}

auto LibvipsImage::size() const -> unsigned long
{
    return _size;
}

auto LibvipsImage::data() const -> const unsigned char*
{
    return _data.get();
}

auto LibvipsImage::framerate() const -> int
{
    // only return framerate if it is webp or gif
    std::unordered_set<std::string> supported_formats {
        ".gif", ".webp"
    };
    if (!supported_formats.contains(path.extension())) return -1;
    try {
        return image.get_int("n-pages");
    } catch (const VError& err) {
        return -1;
    }
}
