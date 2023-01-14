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
    std::cout << "Parent: " << parent << " PID: " << pid << std::endl;
}

Terminal::~Terminal()
{}

auto Terminal::create_window() -> void
{
    this->window = std::make_unique<Window>(this->connection, this->screen, this->parent);
}

auto Terminal::destroy_window() -> void
{
    this->window.reset();
}

auto Terminal::get_window_id() -> xcb_window_t
{
    if (!this->window.get()) return 0;
    return this->window->get_id();
}

