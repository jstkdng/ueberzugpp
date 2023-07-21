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
#include "util.hpp"
#include "dimensions.hpp"
#include "flags.hpp"

#include <unordered_set>
#include <iostream>
#include <string_view>

#include <spdlog/spdlog.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>

OpencvImage::OpencvImage(std::shared_ptr<Dimensions> new_dims, const std::string& filename, bool in_cache):
path(filename),
dims(std::move(new_dims)),
max_width(dims->max_wpixels()),
max_height(dims->max_hpixels()),
in_cache(in_cache)
{
    image = cv::imread(filename, cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        throw std::runtime_error("OpenCV unable to read image");
    }
    logger = spdlog::get("opencv");
    logger->info("Loading image {}", filename);
    flags = Flags::instance();

    process_image();
}

auto OpencvImage::filename() const -> std::string
{
    return path.string();
}

auto OpencvImage::dimensions() const -> const Dimensions&
{
    return *dims;
}

auto OpencvImage::width() const-> int
{
    return image.cols;
}

auto OpencvImage::height() const -> int
{
    return image.rows;
}

auto OpencvImage::size() const -> size_t
{
    return _size;
}

auto OpencvImage::data() const -> const unsigned char*
{
    return image.data;
}

auto OpencvImage::channels() const -> int
{
    return image.channels();
}

// only use opencl if required
auto OpencvImage::resize_image() -> void
{
    if (in_cache) {
        return;
    }
    const auto [new_width, new_height] = get_new_sizes(max_width, max_height, dims->scaler);
    if (new_width <= 0 && new_height <= 0) {
        return;
    }

    const auto opencl_ctx = cv::ocl::Context::getDefault();
    opencl_available = opencl_ctx.ptr() != nullptr;

    if (opencl_available) {
        logger->debug("OpenCL is available");
        image.copyTo(uimage);
        resize_image_helper(uimage, new_width, new_height);
        uimage.copyTo(image);
    } else {
        resize_image_helper(image, new_width, new_height);
    }
}

void OpencvImage::resize_image_helper(cv::InputOutputArray& mat, int new_width, int new_height)
{
    logger->debug("Resizing image");
    cv::resize(mat, mat, cv::Size(new_width, new_height), 0, 0, cv::INTER_AREA);

    if (flags->no_cache) {
        logger->debug("Caching is disabled");
        return;
    }

    const auto save_location = util::get_cache_file_save_location(path);
    try {
        cv::imwrite(save_location, mat);
        logger->debug("Saved resized image");
    } catch (const cv::Exception& ex) {}
}

void OpencvImage::process_image()
{
    resize_image();

    const std::unordered_set<std::string_view> bgra_trifecta = {
        "x11", "chafa", "wayland"
    };

#ifdef ENABLE_OPENGL
    if (flags->use_opengl) {
        cv::flip(image, image, 0);
    }
#endif

    if (image.channels() == 1) {
        cv::cvtColor(image, image, cv::COLOR_GRAY2BGRA);
    }

    if (bgra_trifecta.contains(flags->output)) {
        if (image.channels() == 3) {
            cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
        }
    } else if (flags->output == "kitty") {
        if (image.channels() == 4) {
            cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
        } else {
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        }
    } else if (flags->output == "sixel") {
        if (image.channels() == 4) {
            cv::cvtColor(image, image, cv::COLOR_BGRA2RGB);
        } else {
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
        }
    }
    _size = image.total() * image.elemSize();
}
