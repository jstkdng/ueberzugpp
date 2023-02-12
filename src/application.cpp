#include "application.hpp"
#include "logging.hpp"
#include "process_info.hpp"
#include "os.hpp"

#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

Application::Application():
terminal(ProcessInfo(os::get_pid()))
{
    canvas = Canvas::init(terminal);
}

Application::~Application()
{}

auto Application::execute(const std::string& cmd) -> void
{
    json j;
    try {
        j = json::parse(cmd);
    } catch (const json::parse_error& e) { 
        logger << "There was an error parsing the command." << std::endl;
        return;
    }
    logger << "=== Command received:\n" << j.dump() << std::endl;
    if (j["action"] == "add") {
        int max_width = static_cast<int>(j["max_width"]) * terminal.font_width;
        int max_height = static_cast<int>(j["max_height"]) * terminal.font_height;
        int x = static_cast<int>(j["x"]) * terminal.font_width;
        int y = static_cast<int>(j["y"]) * terminal.font_height;
        canvas->create(x, y, max_width, max_height);
        image = Image::load(j["path"], max_width, max_height);
        canvas->draw(*image);
    } else if (j["action"] == "remove") {
        canvas->clear();
    } else {
        logger << "=== Command not supported!" << std::endl;
    }
}
