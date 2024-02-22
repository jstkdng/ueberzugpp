# Display images inside a terminal Copyright (C) 2023  JustKidding
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.18)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()
cmake_policy(SET CMP0075 NEW)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE
    Debug
    CACHE STRING "Build type.")

project(
  ueberzugpp
  LANGUAGES CXX C
  VERSION 2.9.2)
add_executable(ueberzug)

option(ENABLE_X11 "Enable X11 canvas." ON)
option(ENABLE_XCB_ERRORS "Enable useful logging of XCB errors." OFF)
option(ENABLE_WAYLAND "Enable wayland canvas" OFF)
option(ENABLE_OPENCV "Enable OpenCV image processing." ON)
option(ENABLE_TURBOBASE64 "Enable Turbo-Base64 for base64 encoding." OFF)
option(ENABLE_OPENGL "Enable canvas rendering with OpenGL." OFF)

include(FetchContent)
include(GNUInstallDirs)
include(CheckCXXSymbolExists)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(TBB REQUIRED)

# check if <execution> is available
set(CMAKE_REQUIRED_LIBRARIES TBB::tbb)
check_cxx_symbol_exists(std::execution::par_unseq execution
                        HAVE_STD_EXECUTION_H)
if(HAVE_STD_EXECUTION_H)
  target_compile_definitions(ueberzug PRIVATE HAVE_STD_EXECUTION_H)
endif()

find_package(CLI11 QUIET)
if(NOT CLI11_FOUND)
  if(FETCHCONTENT_FULLY_DISCONNECTED)
    add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/CLI11")
  else()
    FetchContent_Declare(
      cli11
      URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.3.2.tar.gz)
    list(APPEND FC_LIBS cli11)
  endif()
endif()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
  list(APPEND FC_LIBS nlohmann_json)
endif()

find_package(fmt QUIET)
if(NOT fmt_FOUND)
  FetchContent_Declare(
    fmt URL https://github.com/fmtlib/fmt/archive/refs/tags/10.1.0.tar.gz)
  list(APPEND FC_LIBS fmt)
endif()

find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  set(SPDLOG_FMT_EXTERNAL ON)
  FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz)
  list(APPEND FC_LIBS spdlog)
endif()

if(FC_LIBS)
  FetchContent_MakeAvailable(${FC_LIBS})
endif()

if(ENABLE_OPENGL)
  target_compile_definitions(ueberzug PRIVATE ENABLE_OPENGL)
  find_package(OpenGL REQUIRED)
  list(APPEND UEBERZUG_SOURCES "src/util/egl.cpp")
  list(APPEND UEBERZUG_LIBRARIES OpenGL::OpenGL OpenGL::EGL)
endif()

if(ENABLE_X11)
  target_compile_definitions(ueberzug PRIVATE ENABLE_X11)
  pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
  pkg_check_modules(XCBIMAGE REQUIRED IMPORTED_TARGET xcb-image)
  pkg_check_modules(XCBRES REQUIRED IMPORTED_TARGET xcb-res)
  list(APPEND UEBERZUG_SOURCES "src/util/x11.cpp" "src/canvas/x11/x11.cpp"
       "src/canvas/x11/window/x11.cpp")
  list(APPEND UEBERZUG_LIBRARIES PkgConfig::XCB PkgConfig::XCBIMAGE
       PkgConfig::XCBRES)

  if(ENABLE_OPENGL)
    list(APPEND UEBERZUG_SOURCES "src/canvas/x11/window/x11egl.cpp")
  endif()

  if(ENABLE_XCB_ERRORS)
    target_compile_definitions(ueberzug PRIVATE ENABLE_XCB_ERRORS)
    pkg_check_modules(XCBERRORS REQUIRED IMPORTED_TARGET xcb-errors)
    list(APPEND UEBERZUG_LIBRARIES PkgConfig::XCBERRORS)
  endif()
endif()

if(ENABLE_WAYLAND)
  target_compile_definitions(ueberzug PRIVATE ENABLE_WAYLAND)
  find_package(ECM REQUIRED NO_MODULE)
  list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

  find_library(LIBRT rt REQUIRED)
  if(ENABLE_OPENGL)
    find_package(Wayland REQUIRED COMPONENTS Client Egl)
    list(APPEND UEBERZUG_LIBRARIES Wayland::Egl)
    list(APPEND UEBERZUG_SOURCES "src/canvas/wayland/window/waylandegl.cpp")
  else()
    find_package(Wayland REQUIRED COMPONENTS Client)
  endif()
  find_package(WaylandProtocols REQUIRED)
  find_package(WaylandScanner REQUIRED)

  ecm_add_wayland_client_protocol(
    UEBERZUG_SOURCES PROTOCOL
    "${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml" BASENAME
    "xdg-shell")

  list(
    APPEND
    UEBERZUG_SOURCES
    "src/canvas/wayland/wayland.cpp"
    "src/canvas/wayland/config.cpp"
    "src/canvas/wayland/window/shm.cpp"
    "src/canvas/wayland/window/waylandshm.cpp"
    "src/canvas/wayland/config/sway.cpp"
    "src/canvas/wayland/config/hyprland.cpp"
    "src/canvas/wayland/config/wayfire.cpp"
    "src/canvas/wayland/config/dummy.cpp")

  list(APPEND UEBERZUG_LIBRARIES Wayland::Client ${LIBRT})
endif()

if(ENABLE_OPENCV)
  add_compile_definitions(ENABLE_OPENCV)
  find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)

  list(APPEND UEBERZUG_SOURCES "src/image/opencv.cpp")
  list(APPEND UEBERZUG_LIBRARIES opencv_core opencv_imgproc opencv_imgcodecs
       opencv_videoio)
endif()

if(ENABLE_TURBOBASE64)
  add_compile_definitions(ENABLE_TURBOBASE64)
  find_package(turbobase64 QUIET)
  if(NOT turbobase64_FOUND)
    FetchContent_Declare(
      turbobase64
      URL https://github.com/powturbo/Turbo-Base64/archive/refs/tags/2023.08.tar.gz
    )
    FetchContent_Populate(turbobase64)
    add_subdirectory(${turbobase64_SOURCE_DIR} ${turbobase64_BINARY_DIR}
                     EXCLUDE_FROM_ALL)
    list(APPEND UEBERZUG_LIBRARIES base64)
  else()
    add_compile_definitions(WITH_SYSTEM_TURBOBASE64)
    list(APPEND UEBERZUG_LIBRARIES turbo::base64)
  endif()
endif()

set(PROJECT_WARNINGS_CXX -Wall -Wextra -Wpedantic -Werror)

target_compile_options(
  ueberzug PRIVATE $<$<CONFIG:Debug>:
                   $<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS_CXX}> >)

target_compile_definitions(ueberzug PRIVATE $<$<CONFIG:Debug>: DEBUG >)

pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
pkg_check_modules(SIXEL REQUIRED IMPORTED_TARGET libsixel)
pkg_check_modules(CHAFA REQUIRED IMPORTED_TARGET chafa>=1.6)
pkg_check_modules(EXIF REQUIRED IMPORTED_TARGET libexif)

if(APPLE)
  list(APPEND UEBERZUG_SOURCES src/process/apple.cpp)

  find_package(range-v3 QUIET)
  if(NOT range-v3_FOUND)
    FetchContent_Declare(
      range-v3
      URL https://github.com/ericniebler/range-v3/archive/refs/tags/0.12.0.tar.gz
    )
    FetchContent_Populate(range-v3)
    add_subdirectory(${range-v3_SOURCE_DIR} ${range-v3_BINARY_DIR}
                     EXCLUDE_FROM_ALL)
  endif()
  list(APPEND UEBERZUG_LIBRARIES range-v3)
else()
  list(APPEND UEBERZUG_SOURCES src/process/linux.cpp)
endif()

configure_file("include/version.hpp.in" version.hpp)
configure_file("docs/ueberzugpp.1.in" ueberzugpp.1)

list(
  APPEND
  UEBERZUG_SOURCES
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
  "src/canvas/chafa.cpp"
  "src/canvas/sixel.cpp"
  "src/canvas/kitty/kitty.cpp"
  "src/canvas/kitty/chunk.cpp"
  "src/canvas/iterm2/iterm2.cpp"
  "src/canvas/iterm2/chunk.cpp"
  "src/image.cpp"
  "src/image/libvips.cpp")

list(
  APPEND
  UEBERZUG_LIBRARIES
  nlohmann_json::nlohmann_json
  CLI11::CLI11
  Threads::Threads
  fmt::fmt
  spdlog::spdlog
  OpenSSL::Crypto
  TBB::tbb
  PkgConfig::VIPS
  PkgConfig::SIXEL
  PkgConfig::CHAFA
  PkgConfig::EXIF)

target_include_directories(ueberzug PRIVATE "${CMAKE_SOURCE_DIR}/include"
                                            "${PROJECT_BINARY_DIR}")
target_sources(ueberzug PRIVATE ${UEBERZUG_SOURCES})
target_link_libraries(ueberzug PRIVATE ${UEBERZUG_LIBRARIES})
file(CREATE_LINK ueberzug "${PROJECT_BINARY_DIR}/ueberzugpp" SYMBOLIC)

install(TARGETS ueberzug RUNTIME)
install(FILES "${PROJECT_BINARY_DIR}/ueberzugpp" TYPE BIN)
install(FILES "${PROJECT_BINARY_DIR}/ueberzugpp.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(
  FILES "${PROJECT_BINARY_DIR}/ueberzugpp.1"
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
  RENAME ueberzug.1)
