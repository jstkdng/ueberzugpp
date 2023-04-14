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

#ifndef __OPENCV_IMAGE__
#define __OPENCV_IMAGE__

#include "image.hpp"
#include "terminal.hpp"
#include "dimensions.hpp"

#include <string>
#include <opencv2/core.hpp>
#include <filesystem>

namespace fs = std::filesystem;

class OpencvImage : public Image
{
public:
    OpencvImage(const Terminal& terminal, const Dimensions& dimensions, const Flags& flags,
            const std::string& filename, bool in_cache);
    ~OpencvImage();

    auto width() const -> int override;
    auto height() const -> int override;
    auto size() const -> unsigned long override;
    auto data() const -> const unsigned char* override;
    auto channels() const -> int override;

    auto resize_image() -> void override;
private:
    cv::Mat image;
    cv::UMat uimage;

    fs::path path;
    const Terminal& terminal;
    const Flags& flags;
    const Dimensions& dimensions;

    int _width;
    int _height;
    unsigned long _size;
    int max_width;
    int max_height;
    bool in_cache;
    bool opencl_available;

    void process_image();
};

#endif
