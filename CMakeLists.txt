cmake_minimum_required(VERSION 3.2)
project(hyprsic)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)

option(ENABLE_NVIDIA_NVML "Enable NVIDIA GPU monitoring via NVML" ON)

pkg_check_modules(WAYLAND REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(LIBPULSE REQUIRED  libpulse)

find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner REQUIRED)

set(WAYLAND_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/wayland_generated)
file(MAKE_DIRECTORY ${WAYLAND_GENERATED_DIR})

if(ENABLE_NVIDIA_NVML)
    pkg_check_modules(NVML QUIET nvidia-ml)
    if(NVML_FOUND)
        message(STATUS
                "Found NVML: YES (Includes: ${NVML_INCLUDE_DIRS}, Libs: ${NVML_LIBRARIES})")
        list(APPEND APP_INCLUDE_DIRS ${NVML_INCLUDE_DIRS})
        list(APPEND APP_LINK_LIBRARIES ${NVML_LIBRARIES})
        list(APPEND APP_COMPILE_DEFINITIONS "USE_NVML")
    else()
        message(WARNING "NVML (nvidia-ml.pc) not found. NVIDIA GPU monitoring will be disabled.")
    endif()
endif()




execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=pkgdatadir wayland-protocols
    OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(XDG_SHELL_PROTOCOL "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")

add_custom_command(
    OUTPUT xdg-shell-client-protocol.h
    COMMAND wayland-scanner client-header ${XDG_SHELL_PROTOCOL} xdg-shell-client-protocol.h
    DEPENDS ${XDG_SHELL_PROTOCOL}
)

add_custom_command(
    OUTPUT xdg-shell-client-protocol.c
    COMMAND wayland-scanner private-code ${XDG_SHELL_PROTOCOL} xdg-shell-client-protocol.c
    DEPENDS ${XDG_SHELL_PROTOCOL}
)

set(XDG_SOURCES
    xdg-shell-client-protocol.c
)

set(XDG_HEADERS
    xdg-shell-client-protocol.h
)


set(WAYLAND_SOURCES
    src/wayland/manager.cpp
    src/wayland/listener.cpp
    src/wayland/frame_buffer.cpp
)

set(COLLECTOR_SOURCES 
    src/collectors/workspaces/hyprland.cpp
    src/collectors/battery.cpp
    src/collectors/bluetooth.cpp
    src/collectors/disk.cpp
    src/collectors/memory.cpp
    src/collectors/network.cpp
    src/collectors/playing_now.cpp
    src/collectors/sys_load.cpp
)

set(UTIL_SOURCES
    src/utils/dbus_utils.cpp
)

set(SERVICE_SOURCES
    src/services/bluetooth.cpp
)

set(DEBUG_SOURCES
    src/debug/console_preview.cpp
)

set(APP_SOURCES
    src/app/context.cpp
)


include_directories(
        ${WAYLAND_PROTOCOLS_INCLUDE_DIRS}
        ${WAYLAND_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${DBUS_INCLUDE_DIRS}
        ${LIBPULSE_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
)


add_executable(hyprsic
        src/app/main.cpp
        ${APP_SOURCES}
        ${DEBUG_SOURCES}
        ${WAYLAND_SOURCES}
        ${COLLECTOR_SOURCES}
        ${UTIL_SOURCES}
        ${SERVICE_SOURCES}
        ${XDG_SOURCES}
)

target_sources(hyprsic PRIVATE
    ${XDG_HEADERS}
)

target_link_libraries(hyprsic
        ${WAYLAND_PROTOCOLS_LIBRARIES}
        ${WAYLAND_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${DBUS_LIBRARIES}
        ${LIBPULSE_LIBRARIES}
)