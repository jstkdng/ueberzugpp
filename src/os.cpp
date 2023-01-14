#include "os.hpp"

#include <cstdlib>
#include <array>
#include <stdexcept>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>

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
    result.erase(result.length() - 1);
    return result;
}

auto os::getenv(std::string const& var)
    -> std::optional<std::string>
{
    auto res = std::getenv(var.c_str());
    if (res == nullptr) return {};
    return res;
}

int os::get_pid()
{
    return getpid();
}

