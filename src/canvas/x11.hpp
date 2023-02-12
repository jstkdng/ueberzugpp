#ifndef __X11_CANVAS__
#define __X11_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "terminal.hpp"

#include <xcb/xproto.h>
#include <xcb/xcb_image.h>

class X11Canvas : public Canvas
{
public:
    X11Canvas(const Terminal& terminal);
    ~X11Canvas();

    auto create(int max_width, int max_height) -> void override;
    auto draw(const Image& image) -> void override;
    auto clear() -> void override;
    auto quit() -> void override;

private:
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_image_t *xcb_image = nullptr;
    xcb_gcontext_t gc;
};

#endif
