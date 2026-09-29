cmake_minimum_required(VERSION 3.16)
project(crosshair LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(PkgConfig REQUIRED)

pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext)
pkg_check_modules(XRANDR REQUIRED IMPORTED_TARGET xrandr)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(RSVG REQUIRED IMPORTED_TARGET librsvg-2.0)
pkg_check_modules(YAMLCPP REQUIRED IMPORTED_TARGET yaml-cpp)

add_executable(crosshair
    src/main.cpp
    src/display_server.cpp
    src/config.cpp
    src/x11_utils.cpp
    src/overlay_window.cpp
    src/exit_dialog_window.cpp
    src/single_instance.cpp
    src/icon.cpp
)

target_link_libraries(crosshair PRIVATE
    PkgConfig::X11
    PkgConfig::XEXT
    PkgConfig::XRANDR
    PkgConfig::CAIRO
    PkgConfig::RSVG
    PkgConfig::YAMLCPP
)

install(TARGETS crosshair RUNTIME DESTINATION bin)
