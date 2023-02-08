# Display images inside a terminal
# Copyright (C) 2023  JustKidding
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

project(ueberzugpp LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

pkg_check_modules(XCBIMAGE REQUIRED IMPORTED_TARGET xcb-image)

set(UEBERZUGPP_SOURCES
    "src/main.cpp"
    "src/logging.cpp"
    "src/display.cpp"
    "src/image.cpp"
    "src/os.cpp"
    "src/tmux.cpp"
    "src/process_info.cpp"
    "src/util.cpp"
    "src/terminal.cpp"
    "src/window.cpp"
)

add_executable(ueberzug ${UEBERZUGPP_SOURCES})

target_include_directories(ueberzug PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(ueberzug PUBLIC
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    X11::xcb
    Threads::Threads
    PkgConfig::XCBIMAGE
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
)

install(TARGETS ueberzug
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin)

