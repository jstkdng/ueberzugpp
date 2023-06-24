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
#include "os.hpp"
#include "util.hpp"

#include <cmath>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

SixelCanvas::SixelCanvas()
{
    logger = spdlog::get("sixel");
    const auto draw_callback = [] (char *data, int size, void* priv) -> int {
        auto *str = static_cast<std::string*>(priv);
        str->append(data, size);
        return size;
    };
    sixel_output_new(&output, draw_callback, &str, nullptr);
    logger->info("Canvas created");
}

SixelCanvas::~SixelCanvas()
{
    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    sixel_dither_destroy(dither);
    sixel_output_destroy(output);
}

void SixelCanvas::add_image(const std::string& identifier, std::unique_ptr<Image> new_image)
{
    remove_image(identifier);

    image = std::move(new_image);
    const auto dims = image->dimensions();
    x = dims.x + 1;
    y = dims.y + 1;
    horizontal_cells = std::ceil(static_cast<double>(image->width()) / dims.terminal.font_width);
    vertical_cells = std::ceil(static_cast<double>(image->height()) / dims.terminal.font_height);

    const auto file_size = fs::file_size(image->filename());
    const auto reserve_ratio = 50;
    str.reserve(file_size * reserve_ratio);

    // create dither and palette from image
    sixel_dither_new(&dither, -1, nullptr);
    sixel_dither_initialize(dither,
            const_cast<unsigned char*>(image->data()),
            image->width(),
            image->height(),
            SIXEL_PIXELFORMAT_RGB888,
            SIXEL_LARGE_LUM,
            SIXEL_REP_CENTER_BOX,
            SIXEL_QUALITY_HIGH);

    draw();
}

void SixelCanvas::draw()
{
    if (!image->is_animated()) {
        draw_frame();
        return;
    }

    // start drawing loop
    draw_thread = std::thread([&] {
        while (can_draw.load()) {
            draw_frame();
            image->next_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(image->frame_delay()));
        }
    });
}

void SixelCanvas::remove_image([[maybe_unused]] const std::string& identifier)
{
    if (horizontal_cells == 0 && vertical_cells == 0) {
        return;
    }

    can_draw.store(false);
    if (draw_thread.joinable()) {
        draw_thread.join();
    }
    can_draw.store(true);

    sixel_dither_destroy(dither);
    dither = nullptr;

    util::clear_terminal_area(x, y, horizontal_cells, vertical_cells);
    image.reset();
}

void SixelCanvas::draw_frame()
{
    // output sixel content to stream
    sixel_encode(const_cast<unsigned char*>(image->data()),
            image->width(),
            image->height(),
            3 /*unused*/, dither, output);

    util::save_cursor_position();
    util::move_cursor(y, x);
    std::cout << str << std::flush;
    util::restore_cursor_position();
    str.clear();
}
