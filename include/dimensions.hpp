#ifndef __DIMENSIONS__
#define __DIMENSIONS__

#include "terminal.hpp"

class Dimensions
{
public:
    Dimensions(const Terminal& terminal, int x, int y, int max_w, int max_h);
    ~Dimensions() = default;

    int xpixels() const;
    int ypixels() const;
    int max_wpixels() const;
    int max_hpixels() const;

    int x;
    int y;
    int max_w;
    int max_h;

private:
    const Terminal& terminal;
};

#endif
