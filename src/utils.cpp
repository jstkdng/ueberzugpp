#include "utils.hpp"

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
    return result;
}

std::string os::getenv(std::string const& var)
{
    return std::getenv(var.c_str());
}

int os::get_pid()
{
    return getpid();
}

void os::get_process_info(int pid)
{
    std::stringstream ss;
    ss << "/proc/" << pid << "/stat";
    unsigned int n1, n2;
    std::ifstream is(ss.str());
    is >> n1 >> n2;
    std::cout << n1 << " " << n2;
    // read stream size

}

