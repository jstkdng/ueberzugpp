#include "image.hpp"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sys/types.h>
#include <vips/VImage8.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>

namespace fs = std::filesystem;

Image::Image(std::string &filename, xcb_connection_t *connection, xcb_drawable_t drawable):
filename(filename),
connection(connection),
drawable(drawable)
{
    this->image = vips::VImage::thumbnail(filename.c_str(), 300);
}

Image::~Image()
{
    xcb_image_destroy(this->xcb_image);
}

void Image::create_xcb_gc()
{
    xcb_gcontext_t cid = xcb_generate_id(this->connection);
    xcb_create_gc(this->connection, cid, this->drawable, 0, nullptr);
    this->gcontext = cid;
}

void Image::create_xcb_image()
{
    std::size_t len = fs::file_size(filename);
    void *memory = calloc(len, sizeof(char));
    this->xcb_image = xcb_image_create_native(connection,
            this->image.width(),
            this->image.height(),
            XCB_IMAGE_FORMAT_Z_PIXMAP,
            0,
            memory,
            len,
            static_cast<unsigned char*>(this->image.write_to_memory(&len)));
}


void Image::draw()
{
    xcb_image_put(this->connection, this->drawable, this->gcontext, this->xcb_image, 0, 0, 10);
}
