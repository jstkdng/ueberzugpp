#include "process_info.hpp"
#include "util.hpp"

#include <fstream>
#include <string>

const unsigned int MINOR_DEVICE_NUMBER_MASK = 0b11111111111100000000000011111111;

ProcessInfo::ProcessInfo(int pid):
pid(pid)
{
    std::string stat = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream is(stat);
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
    this->tty_nr = std::stoi(proc[4]);
    this->minor_dev = this->tty_nr & MINOR_DEVICE_NUMBER_MASK;
    this->pty_path = "/dev/pts/" + std::to_string(this->minor_dev);

    is.close();
}

ProcessInfo::~ProcessInfo()
{}
