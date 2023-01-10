#ifndef __UTILS__
#define __UTILS__

#include <cstdlib>
#include <glib-2.0/glib.h>
#include <string>

struct free_delete
{
    void operator()(void *x) { free(x); }
};

namespace os
{
    std::string exec(std::string const& cmd);
    std::string getenv(std::string const& var);

    int get_pid();

    void get_process_info(int pid);
}

#endif
