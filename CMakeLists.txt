cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed")

project(ueberzugpp LANGUAGES CXX)

set(UEBERZUGPP_SOURCES
    "src/main.cpp"
    "src/logging.cpp"
)

find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)

include(FindPkgConfig)
pkg_check_modules(VIPS vips-cpp REQUIRED IMPORTED_TARGET)

add_executable(ueberzug ${UEBERZUGPP_SOURCES})

target_include_directories(ueberzug PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(ueberzug PRIVATE nlohmann_json::nlohmann_json PkgConfig::VIPS CLI11::CLI11)
