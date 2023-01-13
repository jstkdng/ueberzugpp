#include "util.hpp"
#include "process_info.hpp"

auto util::str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>
{
    auto start = 0U;
    auto end = str.find(delim);

    std::vector<std::string> res;

    while (end != std::string::npos) {
        res.push_back(str.substr(start, end - start));
        start = end + delim.length();
        end = str.find(delim, start);
    }

    return res;
}

auto util::get_parent_pids(int const& pid) -> std::vector<int>
{
    std::vector<int> res;
    ProcessInfo proc(pid);
    while (proc.ppid != 1) {
        res.push_back(proc.ppid);
        proc = ProcessInfo(proc.ppid);
    }
    return res;
}

