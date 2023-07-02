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
#include "util.hpp"
#include "dimensions.hpp"
#include "flags.hpp"

#include <unordered_set>
#include <algorithm>
#ifdef ENABLE_OPENCV
    #include <opencv2/videoio.hpp>
#endif
#include <gsl/gsl>

using vips::VImage;
using vips::VError;

LibvipsImage::LibvipsImage(std::shared_ptr<Dimensions> new_dims,
            const std::string &filename, bool in_cache):
path(filename),
dims(std::move(new_dims)),
max_width(dims->max_wpixels()),
max_height(dims->max_hpixels()),
in_cache(in_cache)
{
    image = VImage::new_from_file(path.c_str()).colourspace(VIPS_INTERPRETATION_sRGB);
    flags = Flags::instance();
    logger = spdlog::get("vips");
    logger->info("Loading image {}", filename);

    try {
        // animated images should have both n-pages and delay
        npages = image.get_int("n-pages");
        std::ignore = image.get_array_int("delay");
        is_anim = true;
        logger->info("Image is animated");
        auto *opts = VImage::option()->set("n", -1);
        backup = VImage::new_from_file(filename.c_str(), opts)
            .colourspace(VIPS_INTERPRETATION_sRGB);
        orig_height = backup.height() / npages;
        image = backup.crop(0, 0, backup.width(), orig_height);
    } catch (const VError& err) {}

    process_image();
}

auto LibvipsImage::dimensions() const -> const Dimensions&
{
    return *dims;
}

auto LibvipsImage::filename() const -> std::string
{
    return path.string();
}

auto LibvipsImage::width() const -> int
{
    return image.width();
}

auto LibvipsImage::height() const -> int
{
    return image.height();
}

auto LibvipsImage::size() const -> size_t
{
    return _size;
}

auto LibvipsImage::data() const -> const unsigned char*
{
    return _data.get();
}

auto LibvipsImage::channels() const -> int
{
    return image.bands();
}

auto LibvipsImage::is_animated() const -> bool
{
    return is_anim;
}

auto LibvipsImage::next_frame() -> void
{
    if (!is_anim) {
        return;
    }
    top += orig_height;
    if (top == backup.height()) {
        top = 0;
    }
    image = backup.crop(0, top, backup.width(), orig_height);
    process_image();
}

auto LibvipsImage::frame_delay() const -> int
{
    if (!is_anim) {
        return -1;
    }
    try {
        const auto delays = backup.get_array_int("delay");
        const int ms_per_sec = 1000;
        if (delays.at(0) == 0) {
#ifdef ENABLE_OPENCV
            const cv::VideoCapture video (path);
            if (video.isOpened()) {
                return gsl::narrow_cast<int>((1.0 / video.get(cv::CAP_PROP_FPS)) * ms_per_sec);
            }
#endif
            return gsl::narrow_cast<int>((1.0 / npages) * ms_per_sec);
        }
        return delays.at(0);
    } catch (const VError& err) {
        return -1;
    }
}

auto LibvipsImage::resize_image() -> void
{
    if (in_cache) {
        return;
    }
    const auto [new_width, new_height] = get_new_sizes(max_width, max_height, dims->scaler);
    if (new_width == 0 && new_height == 0) {
        return;
    }

    double scale = 0;
    if (new_width > width()) {
        scale = static_cast<double>(new_width) / width();
    } else {
        scale = static_cast<double>(std::min(new_width, width())) / std::max(new_width, width());
    }

    logger->debug("Resizing image");
    image = image.resize(scale);
    if (is_anim || flags->no_cache) {
        return;
    }

    const auto save_location = util::get_cache_file_save_location(path);
    try {
        image.write_to_file(save_location.c_str());
        logger->debug("Saved resized image");
    } catch (const VError& err) {}
}

auto LibvipsImage::process_image() -> void
{
    resize_image();

    const std::unordered_set<std::string_view> bgra_trifecta = {
        "x11", "chafa", "wayland"
    };

    if (bgra_trifecta.contains(flags->output)) {
        // alpha channel required
        if (!image.has_alpha()) {
            const int alpha_value = 255;
            image = image.bandjoin(alpha_value);
        }
        // convert from RGB to BGR
        auto bands = image.bandsplit();
        auto tmp = bands[0];
        bands[0] = bands[2];
        bands[2] = tmp;
        image = VImage::bandjoin(bands);
    } else if (flags->output == "sixel") {
        // sixel expects RGB888
        if (image.has_alpha()) {
            image = image.flatten();
        }
    }
    _size = VIPS_IMAGE_SIZEOF_IMAGE(image.get_image());
    _data.reset(static_cast<unsigned char*>(image.write_to_memory(&_size)));
}
