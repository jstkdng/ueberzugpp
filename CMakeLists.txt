cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

project(ueberzugpp LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips-cpp)
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
)

add_executable(ueberzug ${UEBERZUGPP_SOURCES})

target_include_directories(ueberzug PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(ueberzug PUBLIC
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    X11::xcb
    Threads::Threads
    PkgConfig::VIPS
    PkgConfig::XCBIMAGE
)

