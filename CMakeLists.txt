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

cmake_minimum_required(VERSION 3.18)
include(FetchContent)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type.")

project(ueberzugpp LANGUAGES CXX C VERSION 2.8.5)
add_executable(ueberzug)

option(ENABLE_X11           "Enable X11 canvas." ON)
option(ENABLE_SWAY          "Enable Sway canvas" OFF)
option(ENABLE_OPENCV        "Enable OpenCV image processing." ON)
option(ENABLE_TURBOBASE64   "Enable Turbo-Base64 for base64 encoding." OFF)

if(MSVC)
    target_compile_options(ueberzug PRIVATE
        $<$<CONFIG:Debug>:
            /W4 /WX
        >
    )
else()
    target_compile_options(ueberzug PRIVATE
        $<$<CONFIG:Debug>:
            -Wall -Wextra -Wpedantic -Werror
        >
    )
endif()

target_compile_definitions(ueberzug PRIVATE MAGIC_EXIT_NUM1=69)
target_compile_definitions(ueberzug PRIVATE MAGIC_EXIT_NUM2=420)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(TBB REQUIRED)

find_package(Microsoft.GSL CONFIG QUIET)
if (NOT Microsoft.GSL_FOUND)
    option(GSL_TEST OFF)
    FetchContent_Declare(GSL
        URL https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz
    )
    list(APPEND FC_LIBS GSL)
endif()

find_package(CLI11 QUIET)
if(NOT CLI11_FOUND)
    FetchContent_Declare(cli11
        URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.3.2.tar.gz
    )
    list(APPEND FC_LIBS cli11)
endif()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    list(APPEND FC_LIBS nlohmann_json)
endif()

find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    FetchContent_Declare(spdlog
        URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
    )
    list(APPEND FC_LIBS spdlog)
endif()

find_package(fmt QUIET)
if(NOT fmt_FOUND)
    FetchContent_Declare(fmt
        URL https://github.com/fmtlib/fmt/archive/refs/tags/9.1.0.tar.gz
    )
    list(APPEND FC_LIBS fmt)
endif()

if(FC_LIBS)
    FetchContent_MakeAvailable(${FC_LIBS})
endif()

if(ENABLE_X11)
    target_compile_definitions(ueberzug PRIVATE ENABLE_X11)
    find_package(X11 REQUIRED)
    pkg_check_modules(XCBIMAGE REQUIRED IMPORTED_TARGET xcb-image)
    list(APPEND UEBERZUG_SOURCES
        "src/util/x11.cpp"
        "src/canvas/x11/x11.cpp"
        "src/canvas/x11/window.cpp"
    )
    list(APPEND UEBERZUG_LIBRARIES
        X11::xcb
        PkgConfig::XCBIMAGE
    )
endif()

if(ENABLE_SWAY)
    target_compile_definitions(ueberzug PRIVATE ENABLE_SWAY)
    find_package(ECM REQUIRED NO_MODULE)
    list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

    find_library(LIBRT rt REQUIRED)
    find_package(Wayland REQUIRED COMPONENTS Client)
    find_package(WaylandProtocols REQUIRED)
    find_package(WaylandScanner REQUIRED)

    ecm_add_wayland_client_protocol(UEBERZUG_SOURCES
        PROTOCOL "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml"
        BASENAME "xdg-shell")

    list(APPEND UEBERZUG_SOURCES
        "src/canvas/sway/sway.cpp"
        "src/canvas/sway/shm.cpp"
        "src/canvas/sway/socket.cpp"
    )

    list(APPEND UEBERZUG_LIBRARIES
        Wayland::Client
        ${LIBRT}
    )
endif()

if(ENABLE_OPENCV)
    add_compile_definitions(ENABLE_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)

    list(APPEND UEBERZUG_SOURCES
        "src/image/opencv.cpp"
    )
    list(APPEND UEBERZUG_LIBRARIES
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
            URL https://github.com/powturbo/Turbo-Base64/archive/refs/tags/2023.04.tar.gz
        )
        FetchContent_Populate(turbobase64)
        add_subdirectory(${turbobase64_SOURCE_DIR} ${turbobase64_BINARY_DIR} EXCLUDE_FROM_ALL)
        list(APPEND UEBERZUG_LIBRARIES base64)
    else()
        add_compile_definitions(WITH_SYSTEM_TURBOBASE64)
        list(APPEND UEBERZUG_LIBRARIES turbo::base64)
    endif()
endif()

pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
pkg_check_modules(SIXEL REQUIRED IMPORTED_TARGET libsixel)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)
pkg_check_modules(CHAFA REQUIRED IMPORTED_TARGET chafa>=1.6)

if (APPLE)
    list(APPEND UEBERZUG_SOURCES src/process/apple.cpp)
else()
    list(APPEND UEBERZUG_SOURCES src/process/linux.cpp)
endif()

configure_file("include/version.hpp.in" version.hpp)

list(APPEND UEBERZUG_SOURCES
    "src/main.cpp"
    "src/application.cpp"
    "src/os.cpp"
    "src/tmux.cpp"
    "src/terminal.cpp"
    "src/dimensions.cpp"
    "src/flags.cpp"
    "src/util/util.cpp"
    "src/util/socket.cpp"

    "src/canvas.cpp"
    "src/canvas/sixel.cpp"
    "src/canvas/chafa.cpp"
    "src/canvas/kitty/kitty.cpp"
    "src/canvas/kitty/chunk.cpp"
    "src/canvas/iterm2/iterm2.cpp"
    "src/canvas/iterm2/chunk.cpp"

    "src/image.cpp"
    "src/image/libvips.cpp"
)

list(APPEND UEBERZUG_LIBRARIES
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    Threads::Threads
    spdlog::spdlog
    fmt::fmt
    OpenSSL::Crypto
    Microsoft.GSL::GSL
    TBB::tbb
    PkgConfig::VIPS
    PkgConfig::SIXEL
    PkgConfig::CHAFA
    PkgConfig::UUID
)

target_include_directories(ueberzug PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}"
)
target_sources(ueberzug PRIVATE ${UEBERZUG_SOURCES})
target_link_libraries(ueberzug PRIVATE ${UEBERZUG_LIBRARIES})

install(TARGETS ueberzug
    CONFIGURATIONS Release Debug None
    RUNTIME DESTINATION bin)

