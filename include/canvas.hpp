#ifndef __CANVAS__
#define __CANVAS__

#include "image.hpp"
#include "terminal.hpp"

#include <memory>

class Canvas
{
public:
    static auto init(const Terminal& terminal) -> std::unique_ptr<Canvas>;
    virtual ~Canvas() {}

    virtual auto create(int x, int y, int max_width, int max_height) -> void = 0;
    virtual auto draw(const Image& image) -> void = 0;
    virtual auto clear() -> void = 0;
};

#endif

