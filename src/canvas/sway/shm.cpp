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

#include "shm.hpp"
#include "util.hpp"

#include <algorithm>
#include <ranges>
#include <system_error>

#include <fmt/format.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

SwayShm::SwayShm(int width, int height, struct wl_shm* shm):
shm(shm),
width(width),
height(height),
stride(width * 4),
pool_size(height * stride * 2)
{
    shm_path = fmt::format("/{}", util::generate_uuid_v4());
    create_shm_file();
    allocate_pool_buffers();
}

void SwayShm::create_shm_file()
{
    const mode_t shm_mode = 0600;
    fd = shm_open(shm_path.c_str(), O_RDWR | O_CREAT | O_EXCL, shm_mode);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
    int res = ftruncate(fd, pool_size);
    if (res == -1) {
        throw std::system_error(errno, std::system_category());
    }
    pool_data = reinterpret_cast<uint8_t*>(
        mmap(nullptr, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)
    );
}

void SwayShm::allocate_pool_buffers()
{
    pool = wl_shm_create_pool(shm, fd, pool_size);
    buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
}

auto SwayShm::get_data(uint32_t offset) -> uint32_t*
{
    if (offset > 1) {
        return nullptr;
    }
    return reinterpret_cast<uint32_t*>(&pool_data[offset]);
}

SwayShm::~SwayShm()
{
    shm_unlink(shm_path.c_str());
    close(fd);
    munmap(pool_data, pool_size);
    if (pool != nullptr) {
        wl_shm_pool_destroy(pool);
    }
    if (buffer != nullptr) {
        wl_buffer_destroy(buffer);
    }
}
