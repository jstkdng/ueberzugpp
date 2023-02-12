#include "x11.hpp"

#include <xcb/xcb.h>

X11Canvas::X11Canvas(const Terminal& terminal)
{
    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) {
        throw std::runtime_error("CANNOT CONNECT TO X11");
    }
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
}

X11Canvas::~X11Canvas()
{
    xcb_disconnect(connection);
}

auto X11Canvas::draw(const Image& image) -> void
{
    auto ptr = malloc(image.size());
    xcb_image = xcb_image_create_native(connection,
            image.width(),
            image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            screen->root_depth,
            nullptr,
            image.size(),
            const_cast<unsigned char*>(image.data()));
}

auto X11Canvas::create(int max_width, int max_height) -> void
{};

auto X11Canvas::clear() -> void
{
    if (xcb_image) xcb_image_destroy(xcb_image);
}

auto X11Canvas::quit() -> void
{}
