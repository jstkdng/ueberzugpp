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

#include "chafa.hpp"
#include "util.hpp"
#include "util/ptr.hpp"

#include <cmath>
#include <iostream>
#include <algorithm>

ChafaCanvas::ChafaCanvas()
{
    gchar **envp = g_get_environ();
    term_info = chafa_term_db_detect(chafa_term_db_get_default(), envp);
    g_strfreev(envp);
}

ChafaCanvas::~ChafaCanvas()
{
    chafa_canvas_unref(canvas);
    chafa_canvas_config_unref(config);
    chafa_symbol_map_unref(symbol_map);
    chafa_term_info_unref(term_info);
}

void ChafaCanvas::init(const Dimensions& dimensions, std::unique_ptr<Image> new_image)
{
    image = std::move(new_image);
    x = dimensions.x + 1;
    y = dimensions.y + 1;
    horizontal_cells = std::ceil(static_cast<double>(image->width()) / dimensions.terminal.font_width);
    vertical_cells = std::ceil(static_cast<double>(image->height()) / dimensions.terminal.font_height);

    symbol_map = chafa_symbol_map_new();
    config = chafa_canvas_config_new();
    chafa_symbol_map_add_by_tags(symbol_map, CHAFA_SYMBOL_TAG_ALL);
    chafa_canvas_config_set_symbol_map(config, symbol_map);
    chafa_canvas_config_set_pixel_mode(config, CHAFA_PIXEL_MODE_SYMBOLS);
    chafa_canvas_config_set_geometry(config, horizontal_cells, vertical_cells);
    canvas = chafa_canvas_new(config);

    auto pixel_type = CHAFA_PIXEL_RGB8;
    if (image->channels() == 4) {
        pixel_type = CHAFA_PIXEL_RGBA8_PREMULTIPLIED;
    }
    chafa_canvas_draw_all_pixels(canvas,
            pixel_type,
            image->data(),
            image->width(),
            image->height(),
            image->width() * image->channels());
}

void ChafaCanvas::draw()
{
    const auto result = std::unique_ptr<GString, gstring_deleter> {
        chafa_canvas_print(canvas, term_info)
    };
    auto ycoord = y;
    const auto lines = util::str_split(result->str, "\n");
    util::save_cursor_position();
    std::ranges::for_each(std::as_const(lines), [&] (const auto& line) {
        util::move_cursor(ycoord++, x);
        std::cout << line;
    });
    std::cout << std::flush;
    util::restore_cursor_position();
}

void ChafaCanvas::clear()
{
    if (horizontal_cells == 0 && vertical_cells == 0) {
        return;
    }

    util::clear_terminal_area(x, y, horizontal_cells, vertical_cells);
    chafa_canvas_unref(canvas);
    chafa_canvas_config_unref(config);
    chafa_symbol_map_unref(symbol_map);
    canvas = nullptr;
    config = nullptr;
    symbol_map = nullptr;
}
