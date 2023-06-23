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
#include "flags.hpp"
#include "dimensions.hpp"

#ifdef ENABLE_OPENCV
#   include <opencv2/imgcodecs.hpp>
#endif
#include <vips/vips.h>
#include <unordered_set>
#include <gsl/gsl>

namespace fs = std::filesystem;
using njson = nlohmann::json;

auto Image::load(const njson& command) -> std::unique_ptr<Image>
{
    const std::string& filename = command.at("path");
    if (!fs::exists(filename)) {
        return nullptr;
    }
    const auto flags = Flags::instance();
    std::string image_path = filename;
    bool in_cache = false;
    if (!flags->no_cache) {
        image_path = check_cache(dimensions, filename);
        in_cache = image_path != filename;
    }

#ifdef ENABLE_OPENCV
    if (cv::haveImageReader(image_path) && !flags->no_opencv) {
        try {
            return std::make_unique<OpencvImage>(dimensions, image_path, in_cache);
        } catch (const cv::Exception& ex) {}
    }
#endif
    if (vips_foreign_find_load(image_path.c_str()) != nullptr) {
        try {
            return std::make_unique<LibvipsImage>(dimensions, image_path, in_cache);
        } catch (const vips::VError& err) {}
    }
    return nullptr;
}

auto Image::check_cache(const Dimensions& dimensions, const fs::path& orig_path) -> std::string
{
    const fs::path cache_path = util::get_cache_file_save_location(orig_path);
    if (!fs::exists(cache_path)) {
        return orig_path;
    }

    vips::VImage cache_img;
    try {
        cache_img = vips::VImage::new_from_file(cache_path.c_str());
    } catch (const vips::VError& err) {
        return orig_path;
    }
    const uint32_t img_width = cache_img.width();
    const uint32_t img_height = cache_img.height();
    const uint32_t dim_width = dimensions.max_wpixels(); 
    const uint32_t dim_height = dimensions.max_hpixels();
    const int delta = 10;

    if ((dim_width >= img_width && dim_height >= img_height) &&
        ((dim_width - img_width) <= delta || (dim_height - img_height) <= delta)) {
        return cache_path;
    }
    return orig_path;
}

auto Image::get_new_sizes(double max_width, double max_height, const std::string& scaler) const
    -> std::pair<int, int>
{
    int img_width = width();
    int img_height = height();
    int new_width = 0;
    int new_height = 0;
    double new_scale = 0;
    double width_scale = 0;
    double height_scale = 0;
    double min_scale = 0;
    double max_scale = 0;

    if (img_height > max_height) {
        if (img_width > max_width) {
            width_scale = max_width / img_width;
            height_scale = max_height / img_height;
            min_scale = std::min(width_scale, height_scale);
            max_scale = std::max(width_scale, height_scale);
            if (img_width * max_scale <= max_width && img_height * max_scale <= max_height) {
                new_scale = max_scale;
            } else {
                new_scale = min_scale;
            }
        } else {
            new_scale = max_height / img_height;
        }
    } else if (img_width > max_width) {
        new_scale = max_width / img_width;
    } else if (scaler == "fit_contain" || scaler == "forced_cover") {
        // I believe these should work the same
        new_scale = max_height / img_height;
        if (img_width >= img_height) {
            new_scale = max_width / img_width;
        }
    }
    new_width = gsl::narrow_cast<int>(img_width * new_scale);
    new_height = gsl::narrow_cast<int>(img_height * new_scale);

    return std::make_pair(new_width, new_height);
}

auto Image::get_dimensions(const njson& json) -> std::unique_ptr<Dimensions>
{
    using std::string;
    int xcoord = 0;
    int ycoord = 0;
    int max_width = 0;
    int max_height = 0;
    string width_key = "max_width";
    string height_key = "max_height";
    string scaler = "contain";
    if (json.contains("scaler")) {
        scaler = json["scaler"];
    }
    if (json.contains("width")) {
        width_key = "width";
        height_key = "height";
    }
    if (json[width_key].is_string()) {
        string width = json[width_key];
        string height = json[height_key];
        max_width = std::stoi(width);
        max_height = std::stoi(height);
    } else {
        max_width = json[width_key];
        max_height = json[height_key];
    }
    if (json["x"].is_string()) {
        string xcoords = json["x"];
        string ycoords = json["y"];
        xcoord = std::stoi(xcoords);
        ycoord = std::stoi(ycoords);
    } else {
        xcoord = json["x"];
        ycoord = json["y"];
    }
    return std::make_unique<Dimensions>(*terminal, xcoord, ycoord, max_width, max_height, scaler);
}
