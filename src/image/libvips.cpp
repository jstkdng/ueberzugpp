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
#include "dimensions.hpp"
#include "flags.hpp"
#include "terminal.hpp"
#include "util.hpp"

#include <algorithm>
#include <unordered_set>

#ifdef ENABLE_OPENCV
#  include <opencv2/videoio.hpp>
#endif

using vips::VError;
using vips::VImage;

LibvipsImage::LibvipsImage(std::shared_ptr<Dimensions> new_dims, const std::string &filename, bool in_cache)
    : path(filename),
      dims(std::move(new_dims)),
      max_width(dims->max_wpixels()),
      max_height(dims->max_hpixels()),
      in_cache(in_cache)
{
    image = VImage::new_from_file(path.c_str()).colourspace(VIPS_INTERPRETATION_sRGB);
    flags = Flags::instance();
    logger = spdlog::get("vips");
    logger->info("loading file {}", filename);

    try {
        // animated images should have both n-pages and delay
        npages = image.get_int("n-pages");
        std::ignore = image.get_array_int("delay");
        is_anim = true;
        logger->info("file is an animated image");
        auto *opts = VImage::option()->set("n", -1);
        backup = VImage::new_from_file(filename.c_str(), opts).colourspace(VIPS_INTERPRETATION_sRGB);
        orig_height = backup.height() / npages;
        image = backup.crop(0, 0, backup.width(), orig_height);
    } catch (const VError &err) {
        logger->debug("Failed to process image animation");
    }

    if (!is_anim) {
        image = image.autorot();
    }
    process_image();
}

auto LibvipsImage::dimensions() const -> const Dimensions &
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

auto LibvipsImage::data() const -> const unsigned char *
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
            const cv::VideoCapture video(path);
            if (video.isOpened()) {
                return static_cast<int>((1.0 / video.get(cv::CAP_PROP_FPS)) * ms_per_sec);
            }
#endif
            return static_cast<int>((1.0 / npages) * ms_per_sec);
        }
        return delays.at(0);
    } catch (const VError &err) {
        return -1;
    }
}

auto LibvipsImage::resize_image() -> void
{
    if (in_cache) {
        return;
    }
    const auto [new_width, new_height] = get_new_sizes(max_width, max_height, dims->scaler, flags->scale_factor);
    if (new_width <= 0 && new_height <= 0) {
        // ensure width and height are pair
        if (flags->needs_scaling) {
            const auto curw = width();
            const auto curh = height();
            if ((curw % 2) != 0 || (curh % 2) != 0) {
                auto *opts = VImage::option()
                                 ->set("height", util::round_up(curh, flags->scale_factor))
                                 ->set("size", VIPS_SIZE_FORCE);
                image = image.thumbnail_image(util::round_up(curw, flags->scale_factor), opts);
            }
        }
        return;
    }

    logger->debug("Resizing image");

    auto *opts = VImage::option()->set("height", new_height)->set("size", VIPS_SIZE_FORCE);
    image = image.thumbnail_image(new_width, opts);

    if (is_anim || flags->no_cache) {
        return;
    }

    const auto save_location = util::get_cache_file_save_location(path);
    try {
        image.write_to_file(save_location.c_str());
        logger->debug("Saved resized image");
    } catch (const VError &err) {
        logger->debug("Could not save resized image");
    }
}

auto LibvipsImage::process_image() -> void
{
    resize_image();
    if (flags->origin_center) {
        const double img_width = static_cast<double>(width()) / dims->terminal->font_width;
        const double img_height = static_cast<double>(height()) / dims->terminal->font_height;
        dims->x -= std::floor(img_width / 2);
        dims->y -= std::floor(img_height / 2);
    }

    const std::unordered_set<std::string_view> bgra_trifecta = {"x11", "chafa", "wayland"};

#ifdef ENABLE_OPENGL
    if (flags->use_opengl) {
        image = image.flipver();
    }
#endif

    if (bgra_trifecta.contains(flags->output)) {
        // alpha channel required
        if (!image.has_alpha()) {
            const int alpha_value = 255;
            image = image.bandjoin(alpha_value);
        }
        // convert from RGB to BGR
        auto bands = image.bandsplit();
        std::swap(bands[0], bands[2]);
        image = VImage::bandjoin(bands);
    } else if (flags->output == "sixel") {
        // sixel expects RGB888
        if (image.has_alpha()) {
            image = image.flatten();
        }
    }
    _size = VIPS_IMAGE_SIZEOF_IMAGE(image.get_image());
    _data.reset(static_cast<unsigned char *>(image.write_to_memory(&_size)));
}
