#include "../../include/arolloa.h"
#include <wlr/version.h>

#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

namespace {
#if defined(WLR_COMPOSITOR_VERSION)
constexpr uint32_t DEFAULT_COMPOSITOR_VERSION = WLR_COMPOSITOR_VERSION;
#else
constexpr uint32_t DEFAULT_COMPOSITOR_VERSION = 5;
#endif

#if defined(WLR_XDG_SHELL_VERSION)
constexpr uint32_t DEFAULT_XDG_VERSION = WLR_XDG_SHELL_VERSION;
#else
constexpr uint32_t DEFAULT_XDG_VERSION = 5;
#endif


struct wlr_backend *autocreate_backend(ArolloaServer *server) {
    if (!server) {
        return nullptr;
    }

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (18 << 8) | 0)
    server->session = nullptr;
    struct wl_event_loop *loop = server->wl_display ? wl_display_get_event_loop(server->wl_display) : nullptr;
    return wlr_backend_autocreate(loop, &server->session);
#elif defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    server->session = nullptr;
    return wlr_backend_autocreate(server->wl_display, &server->session);
#else
    return wlr_backend_autocreate(server->wl_display);
#endif
}

struct wlr_compositor *create_compositor(struct wl_display *display, struct wlr_renderer *renderer) {
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    return wlr_compositor_create(display, DEFAULT_COMPOSITOR_VERSION, renderer);
#else
    return wlr_compositor_create(display, renderer);
#endif
}

struct wlr_xdg_shell *create_xdg_shell(struct wl_display *display) {
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    return wlr_xdg_shell_create(display, DEFAULT_XDG_VERSION);
#else
    return wlr_xdg_shell_create(display);
#endif
}

void destroy_display(ArolloaServer *server) {
    if (!server || !server->wl_display) {
        return;
    }

    wl_display_destroy_clients(server->wl_display);
    wl_display_destroy(server->wl_display);
    server->wl_display = nullptr;
    server->compositor = nullptr;
    server->xdg_shell = nullptr;
    server->decoration_manager = nullptr;

}

void ensure_runtime_dir() {
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0] != '\0') {
        return;
    }

    const char *user = getenv("USER");
    if (!user) {
        user = "arolloa";
    }

    std::string fallback = std::string("/tmp/arolloa-runtime-") + user;
    setenv("XDG_RUNTIME_DIR", fallback.c_str(), 1);
    mkdir(fallback.c_str(), 0700);
}

bool setup_debug_environment(ArolloaServer *server) {
    if (!server->debug_mode) {
        server->nested_backend_active = false;
        return true;
    }

    if (!getenv("WLR_BACKENDS")) {
        setenv("WLR_BACKENDS", "wayland", 1);
    }
    if (!getenv("WLR_WL_OUTPUTS")) {
        setenv("WLR_WL_OUTPUTS", "1", 1);
    }
    if (!getenv("WLR_RENDERER_ALLOW_SOFTWARE")) {
        setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
    }

    server->nested_backend_active = true;
    return true;
}

void destroy_decoration_manager(struct wlr_xdg_decoration_manager_v1 *manager) {
    if (!manager) {
        return;
    }

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (18 << 8) | 0)
    wlr_xdg_decoration_manager_v1_destroy(manager);
#else
    (void)manager;
#endif
}

void destroy_xdg_shell(struct wlr_xdg_shell *shell) {
    if (!shell) {
        return;
    }

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (18 << 8) | 0)
    wlr_xdg_shell_destroy(shell);
#else
    (void)shell;
#endif
}

void destroy_compositor(struct wlr_compositor *compositor) {
    if (!compositor) {
        return;
    }

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (18 << 8) | 0)
    wlr_compositor_destroy(compositor);
#else
    (void)compositor;
#endif
}
} // namespace

void server_init(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->initialized = false;
    server->session = nullptr;
    server->allocator = nullptr;

    ensure_runtime_dir();
    setup_debug_environment(server);

    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "Failed to create Wayland display");
        return;
    }

    server->backend = autocreate_backend(server);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "Failed to create wlroots backend");
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif

        destroy_display(server);

        return;
    }

    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create renderer");
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;

        destroy_display(server);

        return;
    }

    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    if (!server->allocator) {
        wlr_log(WLR_ERROR, "Failed to create allocator");
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;

        destroy_display(server);

        return;
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->compositor = create_compositor(server->wl_display, server->renderer);
    if (!server->compositor) {
        wlr_log(WLR_ERROR, "Failed to create compositor global");
        wlr_allocator_destroy(server->allocator);
        server->allocator = nullptr;
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;

        destroy_display(server);

        return;
    }

    server->xdg_shell = create_xdg_shell(server->wl_display);
    if (!server->xdg_shell) {
        wlr_log(WLR_ERROR, "Failed to create xdg-shell global");

        server->compositor = nullptr;
        if (server->allocator) {
            wlr_allocator_destroy(server->allocator);
            server->allocator = nullptr;
        }
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;

        return;
    }

    server->decoration_manager = wlr_xdg_decoration_manager_v1_create(server->wl_display);
    server->output_layout = wlr_output_layout_create();

    wl_list_init(&server->outputs);
    wl_list_init(&server->views);
    wl_list_init(&server->keyboards);

    server->layout_mode = WindowLayout::GRID;

    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

    server->new_input.notify = server_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    server->seat = wlr_seat_create(server->wl_display, "seat0");
    server->cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
    if (server->cursor_mgr) {
        wlr_xcursor_manager_load(server->cursor_mgr, 1);
    }

    server->ui_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1920, 1080);
    server->cairo_ctx = cairo_create(server->ui_surface);
    server->pango_layout = pango_cairo_create_layout(server->cairo_ctx);

    PangoFontDescription *desc = pango_font_description_from_string((std::string(SwissDesign::PRIMARY_FONT) + " 10").c_str());
    pango_layout_set_font_description(server->pango_layout, desc);
    pango_font_description_free(desc);

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_log(WLR_ERROR, "Failed to add Wayland socket");
        if (server->cursor_mgr) {
            wlr_xcursor_manager_destroy(server->cursor_mgr);
            server->cursor_mgr = nullptr;
        }
        if (server->seat) {
            wlr_seat_destroy(server->seat);
            server->seat = nullptr;
        }
        if (server->output_layout) {
            wlr_output_layout_destroy(server->output_layout);
            server->output_layout = nullptr;
        }
        if (server->decoration_manager) {
            destroy_decoration_manager(server->decoration_manager);
            server->decoration_manager = nullptr;
        }
        destroy_xdg_shell(server->xdg_shell);
        server->xdg_shell = nullptr;
        destroy_compositor(server->compositor);
        server->compositor = nullptr;
        if (server->allocator) {
            wlr_allocator_destroy(server->allocator);
            server->allocator = nullptr;
        }
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;
        return;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        if (server->cursor_mgr) {
            wlr_xcursor_manager_destroy(server->cursor_mgr);
            server->cursor_mgr = nullptr;
        }
        if (server->seat) {
            wlr_seat_destroy(server->seat);
            server->seat = nullptr;
        }
        if (server->output_layout) {
            wlr_output_layout_destroy(server->output_layout);
            server->output_layout = nullptr;
        }
        if (server->decoration_manager) {
            destroy_decoration_manager(server->decoration_manager);
            server->decoration_manager = nullptr;
        }
        destroy_xdg_shell(server->xdg_shell);
        server->xdg_shell = nullptr;
        destroy_compositor(server->compositor);
        server->compositor = nullptr;
        if (server->allocator) {
            wlr_allocator_destroy(server->allocator);
            server->allocator = nullptr;
        }
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
        if (server->session) {
            wlr_session_destroy(server->session);
            server->session = nullptr;
        }
#endif
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;
        return;
    }

    setenv("WAYLAND_DISPLAY", socket, 1);
    wlr_log(WLR_INFO, "Running Arolloa on WAYLAND_DISPLAY=%s%s", socket,
            server->debug_mode ? " (debug nested mode)" : "");

    schedule_startup_animation(server);
    server->initialized = true;
}
