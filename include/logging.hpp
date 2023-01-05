#ifndef __LOGGING__
#define __LOGGING__

#include <fstream>
#include <iostream>

class Logging
{
public:
    Logging();
    ~Logging();

    template <class T>
    void log(T t);

private:
    std::ofstream logfile;
};

template <class T>
void Logging::log(T t)
{
    //std::cout << t << std::endl;
    this->logfile << t << std::endl;
}

#endif
