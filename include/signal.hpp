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

#ifndef __SIGNAL_SINGLETON__
#define __SIGNAL_SINGLETON__

#include <atomic>
#include <memory>

class SignalSingleton
{
public:
    static auto instance() -> std::shared_ptr<SignalSingleton>
    {
        static std::shared_ptr<SignalSingleton> instance {new SignalSingleton};
        return instance;
    }
    
    SignalSingleton(const SignalSingleton&) = delete;
    SignalSingleton(SignalSingleton&) = delete;
    auto operator=(const SignalSingleton&) -> SignalSingleton& = delete;
    auto operator=(SignalSingleton&) -> SignalSingleton& = delete;

    auto get_stop_flag() -> std::atomic<bool>&
    {
        return stop_flag;
    }

private:
    SignalSingleton() = default;
    std::atomic<bool> stop_flag;
};

#endif
