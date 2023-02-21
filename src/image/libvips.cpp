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
#include <iostream>
#include <opencv2/videoio.hpp>

using namespace vips;

LibvipsImage::LibvipsImage(const Terminal& terminal, const Dimensions& dimensions,
            const std::string &filename, bool is_anim):
terminal(terminal),
path(filename),
is_anim(is_anim),
max_width(dimensions.max_wpixels()),
max_height(dimensions.max_hpixels())
{
    if (is_anim) {
        std::string opts = filename + "[n=-1]";
        backup = VImage::new_from_file(opts.c_str())
            .colourspace(VIPS_INTERPRETATION_sRGB);
        try {
            npages = backup.get_int("n-pages");
        } catch (const VError& err) {
            this->is_anim = false;
            npages = -1;
        }
        if (npages == -1) {
            image = backup;
        } else {
            orig_height = backup.height() / npages;
            image = backup.crop(0, 0, backup.width(), orig_height);
        }
    } else {
        image = VImage::new_from_file(path.c_str())
            .colourspace(VIPS_INTERPRETATION_sRGB);
    }
    process_image();
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

auto LibvipsImage::is_animated() const -> bool
{
    return is_anim;
}

auto LibvipsImage::next_frame() -> void
{
    if (!is_anim) return;
    top += orig_height;
    if (top == backup.height()) top = 0;
    image = backup.crop(0, top, backup.width(), orig_height);
    process_image();
}

auto LibvipsImage::frame_delay() const -> int
{
    if (!is_anim) return -1;
    try {
        auto delays = backup.get_array_int("delay");
        if (delays.at(0) == 0) {
            cv::VideoCapture video (path);
            if (video.isOpened()) {
                return (1.0 / video.get(cv::CAP_PROP_FPS)) * 1000;
            } else {
                return (1.0 / npages) * 1000;
            }
        }
        return delays.at(0);
    } catch (const VError& err) {
        return -1;
    }
}

auto LibvipsImage::process_image() -> void
{
    auto [new_width, new_height] = get_new_sizes(max_width, max_height);
    if (new_width != 0 || new_height != 0) {
        image = image.thumbnail_image(new_width);
    }

    if (terminal.supports_sixel()) {
        // sixel expects RGB888
        if (image.has_alpha()) {
            image = image.flatten();
        }
    } else {
        // alpha channel required
        if (!image.has_alpha()) image = image.bandjoin(255);
        // convert from RGB to BGR
        auto bands = image.bandsplit();
        auto tmp = bands[0];
        bands[0] = bands[2];
        bands[2] = tmp;
        image = VImage::bandjoin(bands);
    }

    _size = VIPS_IMAGE_SIZEOF_IMAGE(image.get_image());
    _data.reset(static_cast<unsigned char*>(image.write_to_memory(&(_size))));
}
