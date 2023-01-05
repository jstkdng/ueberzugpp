#include "logging.hpp"

#include <iostream>
#include <filesystem>

#include <unistd.h>

namespace fs = std::filesystem;

Logging::Logging()
{
    //std::stringstream ss;
    //ss << "ueberzug_" << std::to_string(getpid()) << ".txt";
    fs::path tmp = fs::temp_directory_path() / "ueberzug.log";
    this->logfile = std::ofstream(tmp, std::ios_base::app);
}

Logging::~Logging()
{}

