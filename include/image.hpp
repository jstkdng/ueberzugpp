#ifndef __IMAGE__
#define __IMAGE__

#include <memory>
#include <string>

class Image
{
public:
    static std::unique_ptr<Image> load(const std::string& filename,
            const int& max_width, const int& max_height);
    virtual ~Image() {}

    virtual auto width() -> int = 0;
    virtual auto height() -> int = 0;
    virtual auto size() -> unsigned long = 0;
    virtual auto data() -> unsigned char* = 0;
};

#endif
