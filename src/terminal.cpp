#include "terminal.hpp"

#include <iostream>

Terminal::Terminal(int const& pid,
        xcb_window_t const& parent,
        xcb_connection_t *connection,
        xcb_screen_t *screen):
parent(parent),
proc(ProcessInfo(pid)),
connection(connection),
screen(screen)
{ 
    this->window = std::make_unique<Window>(this->connection, this->screen, this->parent);
}

Terminal::~Terminal()
{}

auto Terminal::get_window_id() -> xcb_window_t
{
    if (!this->window.get()) return 0;
    return this->window->get_id();
}

