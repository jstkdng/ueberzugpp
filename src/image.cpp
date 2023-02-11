#include "image.hpp"
#include "image/opencv.hpp"
#include "image/libvips.hpp"

#include "logging.hpp"

#include <opencv2/imgcodecs.hpp>
#include <vips/vips.h>

std::unique_ptr<Image> Image::load(const std::string& filename,
        const int& max_width, const int& max_height)
{
    if (cv::haveImageReader(filename)) {
        logger << "=== Loading image with opencv" << std::endl;
        return std::make_unique<OpencvImage>(filename, max_width, max_height);
    }
    std::string vips_loader = vips_foreign_find_load(filename.c_str());
    if (!vips_loader.empty()) {
        logger << "=== Loading image with libvips" << std::endl;
        return std::make_unique<LibvipsImage>(filename, max_width, max_height);
    }
    throw std::runtime_error("Image format unsupported.");
}
