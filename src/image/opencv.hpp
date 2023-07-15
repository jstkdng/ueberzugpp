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

#ifndef OPENCV_IMAGE_H
#define OPENCV_IMAGE_H

#include "image.hpp"

#include <string>
#include <opencv2/core.hpp>
#include <filesystem>
#include <spdlog/fwd.h>

namespace fs = std::filesystem;

class Dimensions;
class Flags;

class OpencvImage : public Image
{
public:
    OpencvImage(std::shared_ptr<Dimensions> new_dims, const std::string& filename, bool in_cache);
    ~OpencvImage() override = default;

    [[nodiscard]] auto dimensions() const -> const Dimensions& override;
    [[nodiscard]] auto width() const -> int override;
    [[nodiscard]] auto height() const -> int override;
    [[nodiscard]] auto size() const -> size_t override;
    [[nodiscard]] auto data() const -> const unsigned char* override;
    [[nodiscard]] auto channels() const -> int override;

    [[nodiscard]] auto filename() const -> std::string override;
private:
    cv::Mat image;
    cv::UMat uimage;

    fs::path path;
    std::shared_ptr<Dimensions> dims;

    uint64_t _size = 0;
    uint32_t max_width;
    uint32_t max_height;
    bool in_cache;
    bool opencl_available = false;

    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<Flags> flags;

    void process_image();
    void resize_image();
    void resize_image_helper(cv::InputOutputArray& mat, int new_width, int new_height);
};

#endif
