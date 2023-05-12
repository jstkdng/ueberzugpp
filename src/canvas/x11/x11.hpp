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

#ifndef __X11_CANVAS__
#define __X11_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "window.hpp"
#include "dimensions.hpp"
#include "util/x11.hpp"

#include <memory>
#include <unordered_map>
#include <xcb/xproto.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <spdlog/spdlog.h>

class X11Canvas : public Canvas
{
public:
    explicit X11Canvas(std::mutex& img_lock);
    ~X11Canvas() override;

    void init(const Dimensions& dimensions, std::shared_ptr<Image> image) override;
    void draw() override;
    void clear() override;
    void toggle() override;
    void hide() override;
    void show() override;

private:
    X11Util xutil;
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    std::unordered_map<xcb_window_t, std::unique_ptr<Window>> windows;
    std::shared_ptr<Image> image;
    std::mutex windows_mutex;
    std::mutex &img_lock;

    std::thread draw_thread;
    std::atomic<bool> can_draw {true};
    std::thread event_handler;

    std::shared_ptr<spdlog::logger> logger;

    void handle_events();
    void discard_leftover_events();
};

#endif
