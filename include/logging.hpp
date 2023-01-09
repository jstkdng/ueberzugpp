// Display images inside a terminal
// Copyright (C) 2023  JustKidding
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
    std::cout << t << std::endl;
    this->logfile << t << std::endl;
}

#endif
