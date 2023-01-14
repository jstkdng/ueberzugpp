#include "tmux.hpp"
#include "os.hpp"

#include <sstream>
#include <string>

std::string tmux::get_session_id()
{
    std::string cmd =
        "tmux display -p -F '#{session_id}' -t " + tmux::get_pane();
    return os::exec(cmd);
}

bool tmux::is_used()
{
    return !tmux::get_pane().empty();
}

bool tmux::is_window_focused()
{
    std::string cmd =
        "tmux display -p -F '#{window_active},#{pane_in_mode}' -t " + tmux::get_pane();
    std::string output = os::exec(cmd);
    return output == "1,0";
}

std::string tmux::get_pane()
{
    return os::getenv("TMUX_PANE").value_or("");
}

auto tmux::get_client_pids() -> std::optional<std::vector<int>>
{
    if (!tmux::is_window_focused()) return {};

    std::vector<int> pids;
    std::string cmd =
        "tmux list-clients -F '#{client_pid}' -t " + tmux::get_pane();
    std::string output = os::exec(cmd), to;
    std::stringstream ss(output);

    while(std::getline(ss, to, '\n')) {
        pids.push_back(std::stoi(to));
    }

    return pids;
}

