#include "opencv.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

OpencvImage::OpencvImage(const std::string& filename,
        const int& max_width, const int& max_height):
filename(filename),
max_width(max_width),
max_height(max_height)
{
    image = cv::imread(filename, cv::IMREAD_COLOR);
    _width = image.cols;
    _height = image.rows;

    unsigned long max_dim = (_width >= _height) ?
                            _width : _height;
    unsigned long new_width = 0, new_height = 0;
    double scale = static_cast<double>(max_width) / max_dim;
    if (!(_width <= max_width && _height <= max_height)) {
        if (_width >= _height) {
            new_width = this->max_width;
            new_height = _height * scale;
        } else {
            new_height = this->max_width;
            new_width = _width * scale;
        }
    }

    if (new_width != 0 || new_height != 0) {
        cv::resize(image, image, cv::Size(new_width, new_height),
                0, 0, cv::INTER_AREA);
        _width = new_width;
        _height = new_height;
    }
    // alpha channel required
    if (image.channels() <= 3) {
        cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
    }
    _size = image.total() * image.elemSize();
}

auto OpencvImage::width() -> int
{
    return _width;
}

auto OpencvImage::height() -> int
{
    return _height;
}

auto OpencvImage::size() -> unsigned long
{
    return _size;
}

auto OpencvImage::data() -> unsigned char*
{
    return image.ptr();
}
