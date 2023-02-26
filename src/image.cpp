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
#include "os.hpp"
#include "util.hpp"

#include <opencv2/imgcodecs.hpp>
#include <vips/vips8>
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace fs = std::filesystem;

auto Image::load(const Terminal& terminal,
            const Dimensions& dimensions, const std::string& filename, spdlog::logger& logger)
    -> std::shared_ptr<Image>
{
    if (!fs::exists(filename)) return nullptr;
    auto image_path = check_cache(dimensions, filename);
    bool is_anim = false, load_opencv = false, load_libvips = false,
         in_cache = image_path != filename;
    fs::path file = image_path;
    std::unordered_set<std::string> animated_formats {
        ".gif", ".webp"
    };

    if (animated_formats.contains(file.extension())) {
        is_anim = true;
        load_libvips = true;
    } else if (cv::haveImageReader(image_path)) {
        load_opencv = true;
    } else if (vips_foreign_find_load(image_path.c_str()) != nullptr) {
        load_libvips = true;
    }

    if (load_libvips) { 
        logger.info("Loading image with libvips.");
        return std::make_shared<LibvipsImage>(terminal, dimensions, image_path, is_anim, in_cache);
    }
    if (load_opencv) {
        logger.info("Loading image with opencv.");
        return std::make_shared<OpencvImage>(terminal, dimensions, image_path, in_cache);
    }
    return nullptr;
}

auto Image::check_cache(const Dimensions& dimensions, const fs::path& orig_path) -> std::string
{
    std::string cache_filename = util::get_b2_hash(orig_path) + orig_path.extension().string(),
                cache_dir = util::get_cache_path();
    fs::path cache_path = cache_dir + cache_filename;
    if (!fs::exists(cache_path)) return orig_path;
    auto cache_img = vips::VImage::new_from_file(cache_path.c_str());
    if (cache_img.width() == dimensions.max_wpixels() ||
        cache_img.height() == dimensions.max_hpixels()) {
        return cache_path;
    }
    return orig_path;
}

auto Image::get_new_sizes(double max_width, double max_height)
    -> std::pair<const int, const int>
{
    int _width = width(), _height = height(), new_width, new_height;
    double new_scale = 0, width_scale, height_scale, min_scale, max_scale;

    if (_height > max_height) {
        if (_width > max_width) {
            width_scale = max_width / _width;
            height_scale = max_height / _height;
            min_scale = std::min(width_scale, height_scale);
            max_scale = std::max(width_scale, height_scale);
            if (_width * max_scale <= max_width && _height * max_scale <= max_height) {
                new_scale = max_scale;
            } else {
                new_scale = min_scale;
            }
        } else {
            new_scale = max_height / _height;
        }
    } else if (_width > max_width) {
        new_scale = max_width / _width;
    }
    new_width = _width * new_scale;
    new_height = _height * new_scale;

    return std::make_pair(new_width, new_height);
}
