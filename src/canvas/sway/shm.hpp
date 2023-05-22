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

#ifndef __SWAY_SHM__
#define __SWAY_SHM__

#include <wayland-client.h>
#include <string>
#include <vector>

class SwayShm
{
public:
    SwayShm(int width, int height);
    ~SwayShm();
    void allocate_pool_buffers();
    auto get_data(uint32_t offset = 0) -> uint32_t*;

    wl_shm* shm = nullptr;
private:
    void create_shm_file();

    wl_shm_pool* pool = nullptr;

    int fd = 0;
    std::string shm_path;
    uint8_t *pool_data;
    std::vector<wl_buffer*> buffers;

    const int width = 0;
    const int height = 0;
    const int stride = 0;
    const int pool_size = 0;
};

#endif
