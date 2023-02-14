#include "logging.hpp"
#include "canvas.hpp"
#include "canvas/sixel.hpp"
#include "canvas/x11/x11.hpp"

auto Canvas::init(const Terminal& terminal) -> std::unique_ptr<Canvas>
{
    if (SixelCanvas::is_supported(terminal)) {
        logger << "=== Using sixel output" << std::endl;
        return std::make_unique<SixelCanvas>();
    }
    logger << "=== Using X11 output" << std::endl;
    return std::make_unique<X11Canvas>(terminal);
}
