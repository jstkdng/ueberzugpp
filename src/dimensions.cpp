#include "dimensions.hpp"
#include "tmux.hpp"

Dimensions::Dimensions(const Terminal& terminal, int x, int y, int max_w, int max_h):
terminal(terminal),
x(x), y(y), max_w(max_w), max_h(max_h)
{
    auto [offset_x, offset_y] = tmux::get_offset();
    this->x += offset_x;
    this->y += offset_y;
}

int Dimensions::xpixels() const
{
    return x * terminal.font_width;
}

int Dimensions::ypixels() const
{
    return y * terminal.font_height;
}

int Dimensions::max_wpixels() const
{
    return max_w * terminal.font_width;
}

int Dimensions::max_hpixels() const
{
    return max_h * terminal.font_height;
}
