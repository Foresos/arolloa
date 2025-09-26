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

# wlroots 0.18 tweaked a number of APIs (for example, the output layout factory
# now requires a wl_display argument).  We can still target 0.17 for
# distribution packages, while retaining the ability to fetch 0.18 when newer
# features are available.  Some distributions ship parallel pkg-config files
# ("wlroots-0.18" alongside "wlroots"), while others only expose the generic
# name with versioned metadata.  Try both variants and fall back to fetching
# wlroots automatically if no suitable development package is available.
set(WLROOTS_MIN_VERSION 0.17)

pkg_check_modules(WLROOTS_018 QUIET IMPORTED_TARGET wlroots-0.18)
if (WLROOTS_018_FOUND)
    set(WLROOTS_TARGET PkgConfig::WLROOTS_018)
else()
    pkg_check_modules(WLROOTS QUIET IMPORTED_TARGET "wlroots>=${WLROOTS_MIN_VERSION}")
    if (WLROOTS_FOUND)
        set(WLROOTS_TARGET PkgConfig::WLROOTS)
    endif()
endif()

if (NOT WLROOTS_TARGET)
    set(WLROOTS_VENDOR_ROOT ${CMAKE_BINARY_DIR}/cmake/libs/wlroots)
    set(WLROOTS_FALLBACK_DIR ${WLROOTS_VENDOR_ROOT}/source)
    set(WLROOTS_FALLBACK_INSTALL ${WLROOTS_VENDOR_ROOT}/install)

    include(FetchContent)
    FetchContent_Declare(
        wlroots
        GIT_REPOSITORY https://gitlab.freedesktop.org/wlroots/wlroots.git
        GIT_TAG 0.18.1
    )
    FetchContent_Populate(wlroots)

    find_program(MESON_EXECUTABLE meson)
    if (NOT MESON_EXECUTABLE)
        message(FATAL_ERROR "meson not found. Install meson to build wlroots locally or provide wlroots ${WLROOTS_MIN_VERSION}+ via pkg-config.")
    endif()

    find_program(NINJA_EXECUTABLE ninja)
    if (NOT NINJA_EXECUTABLE)
        message(FATAL_ERROR "ninja not found. Install ninja-build to build wlroots locally or provide wlroots ${WLROOTS_MIN_VERSION}+ via pkg-config.")
    endif()

    include(ExternalProject)
    ExternalProject_Add(wlroots_external
        SOURCE_DIR ${wlroots_SOURCE_DIR}
        BINARY_DIR ${WLROOTS_VENDOR_ROOT}/build
        CONFIGURE_COMMAND
            ${MESON_EXECUTABLE} setup ${WLROOTS_VENDOR_ROOT}/build ${wlroots_SOURCE_DIR}
            --prefix ${WLROOTS_FALLBACK_INSTALL}
            --buildtype release
            --default-library shared
        BUILD_COMMAND ${MESON_EXECUTABLE} compile -C ${WLROOTS_VENDOR_ROOT}/build
        INSTALL_COMMAND ${MESON_EXECUTABLE} install -C ${WLROOTS_VENDOR_ROOT}/build
        BUILD_BYPRODUCTS ${WLROOTS_FALLBACK_INSTALL}/lib/pkgconfig/wlroots.pc
    )

    set(WLROOTS_IMPORTED libwlroots)
    add_library(${WLROOTS_IMPORTED} SHARED IMPORTED)
    add_dependencies(${WLROOTS_IMPORTED} wlroots_external)
    set_target_properties(${WLROOTS_IMPORTED} PROPERTIES
        IMPORTED_LOCATION ${WLROOTS_FALLBACK_INSTALL}/lib/libwlroots.so
        INTERFACE_INCLUDE_DIRECTORIES ${WLROOTS_FALLBACK_INSTALL}/include
    )

    set(WLROOTS_TARGET ${WLROOTS_IMPORTED})
    set(WLROOTS_FALLBACK TRUE)
endif()
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)

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

if (WLROOTS_FALLBACK)
    add_dependencies(arolloa-compositor wlroots_external)
endif()

target_link_libraries(arolloa-compositor
    PRIVATE
        ${WLROOTS_TARGET}
        arolloa_protocols
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

target_link_libraries(arolloa-settings
    PRIVATE
        PkgConfig::GTKMM
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
