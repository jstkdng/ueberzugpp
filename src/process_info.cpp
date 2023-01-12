#include "process_info.hpp"
#include "util.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

ProcessInfo::ProcessInfo(int pid):
pid(pid)
{
    std::stringstream ss;
    ss << "/proc/" << pid << "/stat";

    std::ifstream is(ss.str());
    std::string out;
    std::getline(is, out);

    int start = out.find('(') + 1, end = out.find(')');
    this->executable = out.substr(start, end - start);

    // remove pid and executable from string
    end += 2;
    out = out.substr(end, out.size() - end);
    auto proc = util::str_split(out, " ");
    this->state = proc[0][0];
    this->ppid = std::stoi(proc[1]);

    is.close();
}

ProcessInfo::~ProcessInfo()
{}
