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

#ifndef __IMAGE__
#define __IMAGE__

#include <memory>
#include <string>
#include <filesystem>
#include <spdlog/spdlog.h>
#include "terminal.hpp"
#include "dimensions.hpp"

class Image
{
public:
    static auto load(const Terminal& terminal,
            const Dimensions& dimensions, const std::string& filename, spdlog::logger& logger)
        -> std::shared_ptr<Image>;
    static auto check_cache(const Dimensions& dimensions,
            const std::filesystem::path& orig_path) -> std::string;

    virtual ~Image() = default;

    virtual auto width() const -> int = 0;
    virtual auto height() const -> int = 0;
    virtual auto size() const -> unsigned long = 0;
    virtual auto data() const -> const unsigned char* = 0;

    virtual auto frame_delay() const -> int { return -1; }
    virtual auto next_frame() -> void {}
    virtual auto is_animated() const -> bool { return false; }

protected:
    auto get_new_sizes(double max_width, double max_height)
        -> std::pair<const int, const int>;
};

#endif
