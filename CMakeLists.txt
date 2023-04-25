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

cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type.")

project(ueberzugpp LANGUAGES CXX VERSION 2.7.7)

option(ENABLE_X11           "Enable X11 canvas." ON)
option(ENABLE_OPENCV        "Enable OpenCV image processing." ON)
option(ENABLE_TURBOBASE64   "Enable Turbo-Base64 for base64 encoding." OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

FetchContent_Declare(
    cli11
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.3.2.tar.gz
    FIND_PACKAGE_ARGS NAMES CLI11
)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    FIND_PACKAGE_ARGS NAMES nlohmann_json
)

FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
    FIND_PACKAGE_ARGS NAMES spdlog
)

FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/9.1.0.tar.gz
    FIND_PACKAGE_ARGS NAMES fmt
)

FetchContent_MakeAvailable(cli11 nlohmann_json spdlog fmt)

if(ENABLE_X11)
    add_compile_definitions(ENABLE_X11)
    find_package(X11 REQUIRED)
    pkg_check_modules(XCBIMAGE REQUIRED IMPORTED_TARGET xcb-image)
    set(X11_SOURCES
        "src/util/x11.cpp"
        "src/canvas/x11/x11.cpp"
        "src/canvas/x11/window.cpp"
    )
    set(X11_LIBRARIES
        X11::xcb
        PkgConfig::XCBIMAGE
    )
endif()

if(ENABLE_OPENCV)
    add_compile_definitions(ENABLE_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
    set(OPENCV_SOURCES
        "src/image/opencv.cpp"
    )
    set(OPENCV_LIBRARIES
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        opencv_videoio
    )
endif()

if(ENABLE_TURBOBASE64)
    add_compile_definitions(ENABLE_TURBOBASE64)
    find_package(turbobase64 QUIET)
    if (NOT turbobase64_FOUND)
        FetchContent_Declare(
            turbobase64
            GIT_REPOSITORY https://github.com/powturbo/Turbo-Base64.git
            GIT_TAG        c087948eb84b83e6a0f6e9135aa8c527ffe1b2c0 # release 2023.04
        )
        FetchContent_Populate(turbobase64)
        add_subdirectory(${turbobase64_SOURCE_DIR} ${turbobase64_BINARY_DIR} EXCLUDE_FROM_ALL)
        set(B64_LIBRARIES base64)
    else()
        add_compile_definitions(WITH_SYSTEM_TURBOBASE64)
        set(B64_LIBRARIES turbo::base64)
    endif()
endif()

pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
pkg_check_modules(SIXEL REQUIRED IMPORTED_TARGET libsixel)
pkg_check_modules(ZMQ REQUIRED IMPORTED_TARGET libzmq)

configure_file("include/version.hpp.in" version.hpp)

set(UEBERZUGPP_SOURCES
    "src/main.cpp"
    "src/application.cpp"
    "src/os.cpp"
    "src/tmux.cpp"
    "src/process.cpp"
    "src/terminal.cpp"
    "src/dimensions.cpp"
    "src/flags.cpp"
    "src/util/util.cpp"

    "src/canvas.cpp"
    "src/canvas/sixel.cpp"
    "src/canvas/kitty.cpp"
    "src/canvas/iterm2.cpp"

    "src/image.cpp"
    "src/image/libvips.cpp"

    ${X11_SOURCES}
    ${OPENCV_SOURCES}
)

add_executable(ueberzug ${UEBERZUGPP_SOURCES})

target_include_directories(ueberzug PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${turbobase64_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
)
target_link_libraries(ueberzug PUBLIC
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    Threads::Threads
    spdlog::spdlog
    fmt::fmt
    OpenSSL::Crypto
    PkgConfig::VIPS
    PkgConfig::SIXEL
    PkgConfig::ZMQ
    ${B64_LIBRARIES}
    ${X11_LIBRARIES}
    ${OPENCV_LIBRARIES}
)

install(TARGETS ueberzug
    CONFIGURATIONS Release Debug None
    RUNTIME DESTINATION bin)

