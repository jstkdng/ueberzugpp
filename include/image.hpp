#ifndef __IMAGE__
#define __IMAGE__

#include <string>
#include <vips/vips8>
#include <xcb/xcb_image.h>

class Image
{
public:
    Image(std::string &filename, xcb_connection_t *connection, xcb_drawable_t drawable);
    ~Image();
    void draw();

private:
    void create_xcb_image();
    void create_xcb_gc();

    std::string &filename;
    vips::VImage image;
    
    xcb_gcontext_t gcontext;
    xcb_drawable_t drawable;
    xcb_image_t *xcb_image;
    xcb_connection_t *connection;
};

#endif
