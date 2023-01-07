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

#include <memory>
#include <filesystem>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>

#include "display.hpp"

namespace fs = std::filesystem;

struct free_delete
{
    void operator()(void *x) { free(x); }
};

Display::Display(Logging &logger, std::string &filename):
logger(logger),
filename(filename)
{
    this->connection = xcb_connect(NULL, NULL);
    this->set_screen();
    this->image = vips::VImage::thumbnail(filename.c_str(), 500);
    this->create_xcb_image();
}

Display::~Display()
{
    xcb_unmap_window(this->connection, this->window);
    xcb_destroy_window(this->connection, this->window);
    xcb_free_colormap(this->connection, this->colormap);
    xcb_free_pixmap(this->connection, this->pixmap);
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

void Display::create_colormap()
{
    xcb_colormap_t mid = xcb_generate_id(this->connection);
    xcb_create_colormap(this->connection,
            XCB_COLORMAP_ALLOC_NONE,
            mid,
            this->screen->root,
            this->screen->root_visual);
    this->colormap = mid;
}

void Display::create_pixmap()
{
    xcb_pixmap_t pixmap = xcb_generate_id(this->connection);
    xcb_create_pixmap(this->connection,
            this->screen->root_depth,
            pixmap,
            this->window,
            this->image.width(),
            this->image.height());
    this->pixmap = pixmap;
}

void Display::create_gc()
{ 
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, this->window, 0, nullptr);
    this->gc = cid;
}

void Display::create_xcb_image()
{
    std::size_t len = fs::file_size(filename);
    void *memory = calloc(len, sizeof(unsigned char));
    this->xcb_image = xcb_image_create_native(this->connection,
            this->image.width(),
            this->image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            this->screen->root_depth,
            memory,
            len,
            static_cast<unsigned char*>(this->image.write_to_memory(&len)));
}

void Display::draw_image()
{ 
    // draw image to pixmap
    xcb_image_put(this->connection, this->pixmap, this->gc, this->xcb_image, 0, 0, 0);
    // draw pixmap on window
    xcb_copy_area(this->connection,
            this->pixmap,
            this->window,
            this->gc,
            0, 0,
            0, 0,
            this->image.width(),
            this->image.height());
    // flush connection
    xcb_flush(this->connection);
}

void Display::create_window()
{
    //this->create_colormap();

    unsigned int value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
    unsigned int value_list[4] = {
        this->screen->black_pixel,
        this->screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS
    };

    xcb_window_t wid = xcb_generate_id(this->connection);
    xcb_create_window(this->connection,
            0,
            wid,
            this->screen->root,
            0, 0,
            this->image.width(), this->image.height(),
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            this->screen->root_visual,
            value_mask,
            value_list);
    xcb_map_window(this->connection, wid);
    xcb_flush(this->connection);

    this->window = wid;
    this->create_pixmap();
    this->create_gc();
}

void Display::handle_events()
{
    while (true) {
        std::unique_ptr<xcb_generic_event_t, free_delete>
            event (xcb_wait_for_event(this->connection));
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                logger.log("Exposing window!");
                this->draw_image();
                break;
            }
            case XCB_KEY_PRESS: {
                logger.log("Key pressed!");
                goto endloop;
            }
            default: {
                logger.log("Unknown event");
                break;
            }
        }
    }
    endloop:
    return;
}

