cmake_minimum_required(VERSION 3.20)
project(Arolloa LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_program(WAYLAND_SCANNER wayland-scanner)
if (NOT WAYLAND_SCANNER)
    message(FATAL_ERROR "wayland-scanner not found. Install wayland-protocols (e.g. sudo apt install wayland-protocols).")
endif()

pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)

pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(XDG_SHELL_XML ${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml)
set(XDG_SHELL_HEADER ${GENERATED_DIR}/xdg-shell-protocol.h)
set(XDG_SHELL_SOURCE ${GENERATED_DIR}/xdg-shell-protocol.c)

add_custom_command(
    OUTPUT ${XDG_SHELL_HEADER} ${XDG_SHELL_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${WAYLAND_SCANNER} server-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_SOURCE}
    DEPENDS ${XDG_SHELL_XML}
    COMMENT "Generating xdg-shell protocol sources"
    VERBATIM
)

set_source_files_properties(${XDG_SHELL_HEADER} ${XDG_SHELL_SOURCE} PROPERTIES GENERATED TRUE)

add_library(arolloa_protocols STATIC ${XDG_SHELL_SOURCE})
set_target_properties(arolloa_protocols PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(arolloa_protocols PUBLIC ${GENERATED_DIR})
target_compile_definitions(arolloa_protocols PRIVATE WLR_USE_UNSTABLE)

add_custom_target(arolloa_protocol_headers DEPENDS ${XDG_SHELL_HEADER})
add_dependencies(arolloa_protocols arolloa_protocol_headers)

add_executable(arolloa-compositor
    src/core/compositor_animation.cpp
    src/core/compositor_input.cpp
    src/core/compositor_main.cpp
    src/core/compositor_output.cpp
    src/core/compositor_server_init.cpp
    src/core/compositor_server_runtime.cpp
    src/core/compositor_server_xdg.cpp
    src/core/config.cpp
)
add_dependencies(arolloa-compositor arolloa_protocol_headers)

target_include_directories(arolloa-compositor
    PRIVATE
        include
        ${GENERATED_DIR}
)

target_compile_definitions(arolloa-compositor PRIVATE WLR_USE_UNSTABLE)
target_compile_options(arolloa-compositor PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(arolloa-compositor
    PRIVATE
        arolloa_protocols
        PkgConfig::WLROOTS
        PkgConfig::WAYLAND_SERVER
        PkgConfig::PANGOCAIRO
        PkgConfig::CAIRO
        PkgConfig::PIXMAN
        PkgConfig::XKBCOMMON
        PkgConfig::EGL
        PkgConfig::GLESV2
        Threads::Threads
        m
        ${CMAKE_DL_LIBS}
)

add_executable(arolloa-settings
    src/settings/settings_simple.cpp
)

target_compile_options(arolloa-settings PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(arolloa-settings PRIVATE "AROLLOA_SETTINGS"
)

set(LAUNCH_SCRIPT ${CMAKE_BINARY_DIR}/launch-arolloa.sh)
configure_file(
    resources/launch-arolloa.sh.in
    ${LAUNCH_SCRIPT}
    @ONLY
    FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)
add_custom_target(arolloa-launcher ALL
    DEPENDS ${LAUNCH_SCRIPT}
)

message(STATUS "Build artifacts will be available in ${CMAKE_BINARY_DIR}")
