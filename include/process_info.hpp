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
    char state;
    std::string executable;
};

#endif
