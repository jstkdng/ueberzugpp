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

using namespace vips;

LibvipsImage::LibvipsImage(const std::string& filename,
        int max_width, int max_height):
filename(filename),
max_width(max_width),
max_height(max_height)
{
    auto check = VImage::new_from_file(filename.c_str());
    VImage img;
    // at least 3 bands are required
    if (check.width() < max_width && check.height() < max_height) {
        // thumbnail not required
        img = check.colourspace(VIPS_INTERPRETATION_sRGB);
    } else {
        img = VImage::thumbnail(filename.c_str(), max_width - 1)
            .colourspace(VIPS_INTERPRETATION_sRGB);
    }
    // alpha channel required
    if (!img.has_alpha()) img = img.bandjoin(255);
    // convert from RGB to BGR
    auto bands = img.bandsplit();
    auto tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;

    image = VImage::bandjoin(bands);
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
    try {
        return image.get_int("n-pages");
    } catch (const VError& err) {
        return -1;
    }
}
