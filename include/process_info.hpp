#ifndef __PROCESS_INFO__
#define __PROCESS_INFO__

#include <string>

class ProcessInfo
{
public:
    ProcessInfo(int pid);
    ~ProcessInfo();

    int pid;
    int ppid;
    unsigned int tty_nr;
    unsigned int minor_dev;
    char state;
    std::string executable;
    std::string pty_path;
};

#endif
