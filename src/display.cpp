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

#include <cstdlib>
#include <memory>
#include <filesystem>
#include <vips/image.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>
#include <iostream>

#include "display.hpp"

namespace fs = std::filesystem;
using namespace vips;

struct free_delete
{
    void operator()(void *x) { free(x); }
};

Display::Display(Logging &logger):
logger(logger)
{
    this->connection = xcb_connect(NULL, NULL);
    this->set_screen();
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_free_gc(this->connection, this->gc);
    xcb_image_destroy(this->xcb_image);
    xcb_disconnect(this->connection);
}

void Display::set_screen()
{
    const xcb_setup_t *setup = xcb_get_setup(this->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    this->screen = iter.data;
}

void Display::create_gc()
{
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, this->window, 0, nullptr);
    this->gc = cid;
}

void Display::load_image(std::string filename)
{
    if (this->xcb_image) {
        xcb_image_destroy(this->xcb_image);
        xcb_free_gc(this->connection, this->gc);
    }
    // TODO: CALCULATE THUMNAIL WIDTH
    auto img = VImage::thumbnail(filename.c_str(), 500);
    img = img.colourspace(VIPS_INTERPRETATION_sRGB);
    // convert RGB TO BGR
    auto bands = img.bandsplit();
    auto tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;
    // ensure BGRX
    if (!img.has_alpha()) {
        bands.push_back(bands[2]);
    }
    img = VImage::bandjoin(bands);

    std::size_t len = fs::file_size(filename);
    void *memory = calloc(len, sizeof(unsigned char));
    this->xcb_image = xcb_image_create_native(this->connection,
            img.width(),
            img.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            memory,
            len,
            static_cast<unsigned char*>(img.write_to_memory(&len)));
    this->trigger_redraw();
    //this->image = this->_image.thumbnail_image(500);
}

void Display::trigger_redraw()
{
    xcb_clear_area(this->connection, true, this->window, 0, 0, 0, 0);
    xcb_flush(this->connection);
}

void Display::draw_image()
{
    if (!xcb_image) return;
    this->create_gc();
    // draw image to window
    xcb_image_put(this->connection, this->window, this->gc, this->xcb_image, 0, 0, 0);
    // flush connection
    xcb_flush(this->connection);
}

void Display::create_window()
{
    unsigned int value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    unsigned int value_list[4] = {
        this->screen->black_pixel,
        this->screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE,
        this->screen->default_colormap
    };

    xcb_window_t wid = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
            this->screen->root_depth,
            wid,
            this->screen->root,
            0, 0,
            1000, 1000,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);
    xcb_map_window(this->connection, wid);
    xcb_flush(this->connection);

    this->window = wid;
}

std::thread Display::spawn_event_handler()
{
    return std::thread([this] {
        this->handle_events();
    });
}

void Display::handle_events()
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete>
            event (xcb_wait_for_event(this->connection));
        auto response = event->response_type & ~0x80;
        switch (response) {
            case XCB_EXPOSE: {
                this->draw_image();
                break;
            }
            default: {
                break;
            }
        }
    }
}

