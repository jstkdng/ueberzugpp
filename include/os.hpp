#ifndef __NAMESPACE_OS__
#define __NAMESPACE_OS__

#include <optional>
#include <string>

namespace os
{
    std::string exec(std::string const& cmd);
    auto getenv(std::string const& var) -> std::optional<std::string>;

    int get_pid();

    void get_process_info(int pid);
}

#endif
