#include "utils.hpp"

#include <cstdlib>
#include <memory>
#include <array>
#include <stdexcept>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>

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
    std::ifstream is(ss.str(), std::ifstream::binary | std::ifstream::ate);
    if (!is) return;
    // read stream size
    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);

    std::cout << length << std::endl;

    // read file contents
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length);

    is.read(buffer.get(), length);
    std::cout << buffer << std::endl;
}

