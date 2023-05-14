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

#ifndef __VIPS_IMAGE__
#define __VIPS_IMAGE__

#include "image.hpp"
#include "terminal.hpp"
#include "dimensions.hpp"
#include "util/ptr.hpp"

#include <string>
#include <vips/vips8>
#include <filesystem>
#include <spdlog/spdlog.h>

class LibvipsImage : public Image
{
public:
    LibvipsImage(const Dimensions& dimensions, const std::string &filename, bool is_anim, bool in_cache);

    [[nodiscard]] auto width() const -> int override;
    [[nodiscard]] auto height() const -> int override;
    [[nodiscard]] auto size() const -> size_t override;
    [[nodiscard]] auto data() const -> const unsigned char* override;
    [[nodiscard]] auto channels() const -> int override;

    void resize_image() override;
    void next_frame() override;
    [[nodiscard]] auto frame_delay() const -> int override;
    [[nodiscard]] auto is_animated() const -> bool override;
    [[nodiscard]] auto filename() const -> std::string override;

private:
    vips::VImage image;
    vips::VImage backup;

    unique_C_ptr<unsigned char> _data;
    std::filesystem::path path;
    const Dimensions& dimensions;

    std::shared_ptr<Flags> flags;
    std::shared_ptr<spdlog::logger> logger;

    uint32_t max_width;
    uint32_t max_height;
    size_t _size;

    // for animated pictures
    int top = 0;
    int orig_height;
    int npages;
    bool is_anim;
    bool in_cache;

    void process_image();
};

#endif
