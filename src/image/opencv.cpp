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

#include "opencv.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

OpencvImage::OpencvImage(const Terminal& terminal, const Dimensions& dimensions,
            const std::string& filename):
terminal(terminal),
path(filename),
max_width(dimensions.max_wpixels()),
max_height(dimensions.max_hpixels())
{
    image = cv::imread(filename, cv::IMREAD_UNCHANGED);
    process_image();
}

auto OpencvImage::width() const-> int
{
    return _width;
}

auto OpencvImage::height() const -> int
{
    return _height;
}

auto OpencvImage::size() const -> unsigned long
{
    return _size;
}

auto OpencvImage::data() const -> const unsigned char*
{
    return image.ptr();
}

auto OpencvImage::process_image() -> void
{
    _width = image.cols;
    _height = image.rows;

    auto [new_width, new_height] = get_new_sizes(max_width, max_height);

    if (new_width != 0 || new_height != 0) {
        cv::resize(image, image, cv::Size(new_width, new_height),
                0, 0, cv::INTER_AREA);
        _width = new_width;
        _height = new_height;
    }

    if (terminal.supports_sixel()) {
        if (image.channels() == 4) {
            cv::cvtColor(image, image, cv::COLOR_BGRA2RGB);
        } else {
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        }
    } else {
        if (image.channels() < 4) {
            // alpha channel required
            cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
        }
    }

    _size = image.total() * image.elemSize();
}
