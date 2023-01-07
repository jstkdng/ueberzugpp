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

#include "display.hpp"
#include <xcb/xcb.h>
#include <xcb/xproto.h>

Display::Display()
{
    this->connection = xcb_connect(NULL, NULL);
    this->set_root_window();
}

Display::~Display()
{
    xcb_disconnect(this->connection);
}

void Display::set_root_window()
{
    const xcb_setup_t *setup = xcb_get_setup(this->connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    this->root_window = iter.data->root;
}

