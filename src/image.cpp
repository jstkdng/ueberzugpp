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

#include "image.hpp"
#include "image/opencv.hpp"
#include "image/libvips.hpp"
#include "logging.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <vips/vips.h>
#include <filesystem>

namespace fs = std::filesystem;

auto Image::load(const std::string& filename, int max_width, int max_height,
        const Terminal& terminal)
    -> std::shared_ptr<Image>
{
    fs::path file = filename;
    if (file.extension() == ".webp") {
        logger << "=== Loading image with libvips" << std::endl;
        return std::make_shared<LibvipsImage>(terminal, filename, max_width, max_height);
    }
    bool is_gif = file.extension() == ".gif";
    if (cv::haveImageReader(filename) || is_gif) {
        logger << "=== Loading image with opencv" << std::endl;
        return std::make_shared<OpencvImage>(terminal, filename, max_width, max_height, is_gif);
    }
    std::string vips_loader = vips_foreign_find_load(filename.c_str());
    if (!vips_loader.empty()) {
        logger << "=== Loading image with libvips" << std::endl;
        return std::make_shared<LibvipsImage>(terminal, filename, max_width, max_height);
    }
    cv::VideoCapture capture(filename, cv::CAP_FFMPEG);
    if (capture.isOpened()) {
        logger << "=== Loading image with opencv" << std::endl;
        return std::make_shared<OpencvImage>(terminal, filename, max_width, max_height, true);
    }
    return nullptr;
}

auto Image::get_new_sizes(int max_width, int max_height) -> std::pair<int, int>
{
    int _width = width(), _height = height(), new_width = 0, new_height = 0;
    double new_scale = 0, width_scale, height_scale, min_scale, max_scale;

    if (_height > max_height) {
        if (_width > max_width) {
            width_scale = static_cast<double>(max_width) / _width;
            height_scale = static_cast<double>(max_height) / _height;
            min_scale = std::min(width_scale, height_scale);
            max_scale = std::max(width_scale, height_scale);
            if (_width * max_scale <= max_width && _height * max_scale <= max_height) {
                new_scale = max_scale;
            } else {
                new_scale = min_scale;
            }
        } else {
            new_scale = static_cast<double>(max_height) / _height;
        }
    } else if (_width > max_width) {
        new_scale = static_cast<double>(max_width) / _width;
    }
    new_width = _width * new_scale;
    new_height = _height * new_scale;

    return std::make_pair(new_width, new_height);
}
