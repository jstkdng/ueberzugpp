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

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXECUTABLE_ENABLE_EXPORTS ON)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type.")

project(wayfire-ueberzugpp LANGUAGES CXX C VERSION 0.0.1)
add_library(ueberzugpp SHARED)

list(APPEND PLUGIN_SOURCES "ueberzugpp.cpp")

find_package(PkgConfig REQUIRED)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(WaylandProtocols REQUIRED)
find_package(WaylandScanner REQUIRED)

ecm_add_wayland_client_protocol(PLUGIN_SOURCES
    PROTOCOL "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml"
    BASENAME "xdg-shell")
file(CREATE_LINK
    "wayland-xdg-shell-client-protocol.h"
    "${PROJECT_BINARY_DIR}/xdg-shell-protocol.h" SYMBOLIC)

pkg_check_modules(WAYFIRE REQUIRED IMPORTED_TARGET wayfire)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(WFCONFIG REQUIRED IMPORTED_TARGET wf-config)

add_compile_definitions(WLR_USE_UNSTABLE WAYFIRE_PLUGIN)
target_compile_options(ueberzugpp PRIVATE
    $<$<CONFIG:Debug>:
        -Wall -Wextra -Wpedantic -Werror
    >
)


target_include_directories(ueberzugpp PRIVATE "${PROJECT_BINARY_DIR}")
target_sources(ueberzugpp PRIVATE ${PLUGIN_SOURCES})
target_link_libraries(ueberzugpp
    PkgConfig::WAYFIRE
    PkgConfig::WLROOTS
    PkgConfig::WFCONFIG)
