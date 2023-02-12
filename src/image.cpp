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
#include <vips/vips.h>
#include <filesystem>

namespace fs = std::filesystem;

auto Image::load(const std::string& filename, int max_width, int max_height)
    -> std::unique_ptr<Image>
{
    fs::path file = filename;
    if (cv::haveImageReader(filename) || file.extension() == "gif") {
        logger << "=== Loading image with opencv" << std::endl;
        return std::make_unique<OpencvImage>(filename, max_width, max_height);
    }
    std::string vips_loader = vips_foreign_find_load(filename.c_str());
    if (!vips_loader.empty()) {
        logger << "=== Loading image with libvips" << std::endl;
        return std::make_unique<LibvipsImage>(filename, max_width, max_height);
    }
    return nullptr;
}
