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

#ifndef __DISPLAY__
#define __DISPLAY__

#include "logging.hpp"
#include "image.hpp"

#include <xcb/xproto.h>
#include <memory>
#include <string>

class Display
{
public:
    Display(Logging &logger, std::string &filename);
    ~Display();

    void create_window();
    void handle_events();

private:
    void set_screen();
    void create_colormap();

    xcb_connection_t *connection;
    xcb_screen_t *screen;

    xcb_window_t window;
    xcb_colormap_t colormap;

    Logging &logger;
    std::unique_ptr<Image> image;
    std::string &filename;
};


#endif
