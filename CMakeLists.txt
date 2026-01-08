cmake_minimum_required(VERSION 3.20)
project(hyprsic)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)

option(ENABLE_NVIDIA_NVML "Enable NVIDIA GPU monitoring via NVML" ON)


pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(GTK_LAYER_SHELL REQUIRED gtk-layer-shell-0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(LIBPULSE REQUIRED  libpulse)



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


set(MODULES_SOURCES 
    src/modules/workspaces/hyprland.cpp
    src/modules/sysinfo/battery.cpp
    src/modules/sysinfo/disk.cpp
    src/modules/sysinfo/memory.cpp
    src/modules/sysinfo/network.cpp
    src/modules/media/playing_now.cpp
    src/modules/sysinfo/sys_load.cpp
    src/modules/sysinfo/stats.cpp
    src/modules/bluetooth/manager.cpp
    src/modules/bluetooth/module.cpp
    src/modules/notifications/notification_manager.cpp
)

set(DATABASE_SOURCES
    src/database/db_manager.cpp
)

set(UTIL_SOURCES
    src/utils/dbus_utils.cpp
)

set(DEBUG_SOURCES
    src/debug/console_preview.cpp
)

set(APP_SOURCES
    src/app/context.cpp
    src/app/window.cpp
)


include_directories(
        ${JSONCPP_INCLUDE_DIRS}
        ${DBUS_INCLUDE_DIRS}
        ${LIBPULSE_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GTK_INCLUDE_DIRS} 
        ${GTK_LAYER_SHELL_INCLUDE_DIRS}
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/thirdparty/SQLiteCpp)

add_executable(hyprsic
        src/app/main.cpp
        ${APP_SOURCES}
        ${DATABASE_SOURCES}
        ${DEBUG_SOURCES}
        ${MODULES_SOURCES}
        ${UTIL_SOURCES}
        ${SERVICE_SOURCES}
        ${XDG_SOURCES}
)

target_sources(hyprsic PRIVATE
    ${XDG_HEADERS}
)

target_link_libraries(hyprsic
        SQLiteCpp
        sqlite3
        ${JSONCPP_LIBRARIES}
        ${DBUS_LIBRARIES}
        ${LIBPULSE_LIBRARIES}
        ${GTK_LIBRARIES} 
        ${GTK_LAYER_SHELL_LIBRARIES}
)