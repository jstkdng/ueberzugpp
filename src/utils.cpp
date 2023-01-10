#include "utils.hpp"

#include <cstdlib>
#include <memory>
#include <array>
#include <stdexcept>

std::string os::exec(std::string const& cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed");
    }
    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    } 
    return result;
}

std::string os::getenv(std::string const& var)
{
    return std::getenv(var.c_str());
}

