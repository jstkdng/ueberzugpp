#include "logging.hpp"
#include "canvas.hpp"
#include "canvas/x11.hpp"
#include "canvas/sixel.hpp"

auto Canvas::init(const Terminal& terminal) -> std::unique_ptr<Canvas>
{
    if (SixelCanvas::is_supported(terminal)) {
        logger << "=== Using sixel output" << std::endl;
        return std::make_unique<SixelCanvas>();
    } else {
        logger << "=== Using X11 output" << std::endl;
        return std::make_unique<X11Canvas>(terminal);
    }
    return nullptr;
}
