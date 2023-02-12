#include "sixel.hpp"
#include "os.hpp"

#include <unordered_set>
#include <string>

auto SixelCanvas::is_supported(const Terminal& terminal) -> bool
{
    std::unordered_set<std::string> supported_terms {
        "contour", "foot"
    };
    return supported_terms.contains(terminal.name);
}

SixelCanvas::SixelCanvas()
{
    sixel_encoder_new(&encoder, nullptr);
}

SixelCanvas::~SixelCanvas()
{
    sixel_encoder_unref(encoder);
}

auto SixelCanvas::create(int max_width, int max_height) -> void
{}

auto SixelCanvas::draw(const Image& image) -> void
{
    sixel_encoder_encode_bytes(encoder,
            const_cast<unsigned char*>(image.data()),
            image.width(),
            image.height(),
            SIXEL_PIXELFORMAT_BGRA8888,
            nullptr,
            (-1));
}

auto SixelCanvas::clear() -> void
{}

auto SixelCanvas::quit() -> void
{}
