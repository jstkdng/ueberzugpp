#include "libvips.hpp"

using namespace vips;

LibvipsImage::LibvipsImage(const std::string& filename,
        const int& max_width, const int& max_height):
filename(filename),
max_width(max_width),
max_height(max_height)
{
    auto check = VImage::new_from_file(filename.c_str());
    VImage img;
    // at least 3 bands are required
    if (check.width() < max_width && check.height() < max_height) {
        // thumbnail not required
        img = check.colourspace(VIPS_INTERPRETATION_sRGB);
    } else {
        img = VImage::thumbnail(filename.c_str(), max_width - 1).colourspace(VIPS_INTERPRETATION_sRGB);
    }
    // alpha channel required
    if (!img.has_alpha()) img = img.bandjoin(255);
    // convert from RGB to BGR
    auto bands = img.bandsplit();
    auto tmp = bands[0];
    bands[0] = bands[2];
    bands[2] = tmp;

    image = VImage::bandjoin(bands);
    _size = VIPS_IMAGE_SIZEOF_IMAGE(image.get_image()); 
}

auto LibvipsImage::width() -> int
{
    return image.width();
}

auto LibvipsImage::height() -> int
{
    return image.height();
}

auto LibvipsImage::size() -> unsigned long
{
    return _size;
}

auto LibvipsImage::data() -> unsigned char*
{
    _data.reset(static_cast<unsigned char*>(image.write_to_memory(&(_size))));
    return _data.get();
}
