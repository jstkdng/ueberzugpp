#ifndef __OPENCV_IMAGE__
#define __OPENCV_IMAGE__

#include "image.hpp"

#include <string>
#include <opencv2/core.hpp>

class OpencvImage : public Image
{
public:
    OpencvImage(const std::string& filename,
            const int& max_width, const int& max_height);

    auto width() -> int override;
    auto height() -> int override;
    auto size() -> unsigned long override;
    auto data() -> unsigned char* override;

private:
    cv::Mat image;

    std::string filename;

    int _width;
    int _height;
    unsigned long _size;
    int max_width;
    int max_height;
};

#endif
