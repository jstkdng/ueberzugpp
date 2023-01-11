#ifndef __NAMESPACE_TMUX__
#define __NAMESPACE_TMUX__

#include <string>
#include <vector>
#include <optional>

namespace tmux
{
    std::string get_session_id();

    std::string get_pane();

    bool is_used();

    bool is_window_focused();

    auto get_client_pids() -> std::optional<std::vector<int>>;
}

#endif
