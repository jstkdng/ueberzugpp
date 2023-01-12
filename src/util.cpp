#include "util.hpp"

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

