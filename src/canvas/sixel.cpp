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

#include "sixel.hpp"
#include "dimensions.hpp"
#include "terminal.hpp"
#include "util.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Sixel::Sixel(std::unique_ptr<Image> new_image, std::mutex *stdout_mutex)
    : image(std::move(new_image)),
      stdout_mutex(stdout_mutex)
{
    const auto dims = image->dimensions();
    x = dims.x + 1;
    y = dims.y + 1;
    horizontal_cells = std::ceil(static_cast<double>(image->width()) / dims.terminal->font_width);
    vertical_cells = std::ceil(static_cast<double>(image->height()) / dims.terminal->font_height);

    const auto draw_callback = [](char *data, int size, void *priv) -> int {
        auto *str = static_cast<std::string *>(priv);
        str->append(data, size);
        return size;
    };
    sixel_output_new(&output, draw_callback, &str, nullptr);

    const auto file_size = fs::file_size(image->filename());
    constexpr auto reserve_ratio = 50;
    str.reserve(file_size * reserve_ratio);

    // create dither and palette from image
    sixel_dither_new(&dither, -1, nullptr);
    sixel_dither_initialize(dither, const_cast<unsigned char *>(image->data()), image->width(), image->height(),
                            SIXEL_PIXELFORMAT_RGB888, SIXEL_LARGE_LUM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGH);
}

Sixel::~Sixel()
{
    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    sixel_dither_destroy(dither);
    sixel_output_destroy(output);

    const std::scoped_lock lock{*stdout_mutex};
    util::clear_terminal_area(x, y, horizontal_cells, vertical_cells);
}

void Sixel::draw()
{
    if (!image->is_animated()) {
        generate_frame();
        return;
    }

    // start drawing loop
    draw_thread = std::thread([this] {
        while (can_draw.load()) {
            generate_frame();
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

void Sixel::generate_frame()
{
    // output sixel content to stream
    sixel_encode(const_cast<unsigned char *>(image->data()), image->width(), image->height(), 3 /*unused*/, dither,
                 output);

    const std::scoped_lock lock{*stdout_mutex};
    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
    str.clear();
}
