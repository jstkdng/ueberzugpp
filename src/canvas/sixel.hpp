#ifndef __SIXEL_CANVAS__
#define __SIXEL_CANVAS__

#include "canvas.hpp"
#include "image.hpp"
#include "terminal.hpp"

#include <sixel.h>

class SixelCanvas : public Canvas
{
public:
    SixelCanvas();
    ~SixelCanvas();

    auto create(int x, int y, int max_width, int max_height) -> void override;
    auto draw(const Image& image) -> void override;
    auto clear() -> void override;

    static auto is_supported(const Terminal& terminal) -> bool;

private:
    sixel_encoder_t *encoder;
};


#endif
