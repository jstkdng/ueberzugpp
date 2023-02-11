#ifndef __VIPS_IMAGE__
#define __VIPS_IMAGE__

#include "image.hpp"

#include <string>
#include <memory>
#include <vips/vips8>

class LibvipsImage : public Image
{
public:
    LibvipsImage(const std::string &filename,
            const int& max_width, const int& max_height);

    auto width() -> int override;
    auto height() -> int override;
    auto size() -> unsigned long override;
    auto data() -> unsigned char* override;

private:
    vips::VImage image;

    std::unique_ptr<unsigned char> _data;
    std::string filename;

    int max_width;
    int max_height;
    unsigned long _size;
};

#endif
