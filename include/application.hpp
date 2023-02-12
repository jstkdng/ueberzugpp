#ifndef __APPLICATION__
#define __APPLICATION__

#include "image.hpp"
#include "canvas.hpp"
#include "terminal.hpp"

#include <string>
#include <memory>

class Application
{
public:
    Application();
    ~Application();

    auto execute(const std::string& cmd) -> void;

private:
    Terminal terminal;

    std::unique_ptr<Image> image;
    std::unique_ptr<Canvas> canvas;
};

#endif
