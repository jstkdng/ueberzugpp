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
#ifdef ENABLE_OPENCV
#   include "image/opencv.hpp"
#endif
#include "image/libvips.hpp"
#include "os.hpp"
#include "util.hpp"

#ifdef ENABLE_OPENCV
#   include <opencv2/imgcodecs.hpp>
#endif
#include <vips/vips.h>
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace fs = std::filesystem;

auto Image::load(const Terminal& terminal, const Dimensions& dimensions, const Flags& flags,
        const std::string& filename, spdlog::logger& logger)
    -> std::shared_ptr<Image>
{
    if (!fs::exists(filename)) return nullptr;
    std::string image_path = filename;
    bool in_cache = false;
    if (!flags.no_cache) {
        image_path = check_cache(dimensions, filename);
        in_cache = image_path != filename;
    }
    bool is_anim = false, load_opencv = false, load_libvips = flags.no_opencv;
    fs::path file = image_path;
    std::unordered_set<std::string> animated_formats {
        ".gif", ".webp"
    };

    if (animated_formats.contains(file.extension())) {
        is_anim = true;
        load_libvips = true;
    }
#ifdef ENABLE_OPENCV
    if (cv::haveImageReader(image_path)) {
        load_opencv = true;
    } else if (vips_foreign_find_load(image_path.c_str()) != nullptr) {
        load_libvips = true;
    }
#else
    if (vips_foreign_find_load(image_path.c_str()) != nullptr) {
        load_libvips = true;
    }
#endif

    if (load_libvips) {
        logger.info("Loading image with libvips.");
        return std::make_shared<LibvipsImage>(terminal, dimensions, flags, image_path, is_anim, in_cache);
    }
#ifdef ENABLE_OPENCV
    if (load_opencv) {
        logger.info("Loading image with opencv.");
        return std::make_shared<OpencvImage>(terminal, dimensions, flags, image_path, in_cache);
    }
#endif
    return nullptr;
}

auto Image::check_cache(const Dimensions& dimensions, const fs::path& orig_path) -> std::string
{
    fs::path cache_path = util::get_cache_file_save_location(orig_path);
    if (!fs::exists(cache_path)) return orig_path;

    auto cache_img = vips::VImage::new_from_file(cache_path.c_str());
    int img_width = cache_img.width(), img_height = cache_img.height();

    int dim_width = dimensions.max_wpixels(), dim_height = dimensions.max_hpixels();

    if ((dim_width >= img_width && dim_height >= img_height) &&
        ((dim_width - img_width) <= 10 || (dim_height - img_height) <= 10)) {
        return cache_path;
    }
    return orig_path;
}

auto Image::get_new_sizes(double max_width, double max_height, const std::string& scaler)
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
    } else if (scaler == "fit_contain" || scaler == "forced_cover") {
        // I believe these should work the same
        new_scale = max_height / _height;
        if (_width >= _height) {
            new_scale = max_width / _width;
        }
    }
    new_width = _width * new_scale;
    new_height = _height * new_scale;

    return std::make_pair(new_width, new_height);
}
