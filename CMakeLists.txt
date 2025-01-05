cmake_minimum_required(VERSION 3.2)
project(hyprsic)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client)

include_directories(${WAYLAND_INCLUDE_DIRS})

add_executable(hyprsic
        src/Collectors/Network.cpp
        src/Hyprland/Workspaces.cpp
        src/main.cpp
        src/Collectors/Temperature.cpp
)
target_link_libraries(hyprsic ${WAYLAND_LIBRARIES})
