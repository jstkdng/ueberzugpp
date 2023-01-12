#ifndef __NAMESPACE_UTIL__
#define __NAMESPACE_UTIL__

#include <string>
#include <vector>

namespace util
{
    auto str_split(std::string const& str, std::string const& delim) -> std::vector<std::string>;
}

#endif
