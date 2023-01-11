#ifndef __NAMESPACE_OS__
#define __NAMESPACE_OS__

#include <string>

namespace os
{
    std::string exec(std::string const& cmd);
    std::string getenv(std::string const& var);

    int get_pid();

    void get_process_info(int pid);
}

#endif
