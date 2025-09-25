#include "../../include/arolloa.h"

#include <csignal>
#include <cstdlib>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

namespace {
constexpr uint32_t DEFAULT_COMPOSITOR_VERSION = 5;
constexpr uint32_t DEFAULT_XDG_VERSION = 5;

struct wlr_backend *autocreate_backend(struct wl_display *display) {
    if constexpr (std::is_invocable_v<decltype(&wlr_backend_autocreate), struct wl_display *, struct wlr_session **>) {
        struct wlr_session *session = nullptr;
        return wlr_backend_autocreate(display, &session);
    } else {
        return wlr_backend_autocreate(display);
    }
}

struct wlr_compositor *create_compositor(struct wl_display *display, struct wlr_renderer *renderer) {
    if constexpr (std::is_invocable_v<decltype(&wlr_compositor_create), struct wl_display *, uint32_t, struct wlr_renderer *>) {
        return wlr_compositor_create(display, DEFAULT_COMPOSITOR_VERSION, renderer);
    } else {
        return wlr_compositor_create(display, renderer);
    }
}

struct wlr_xdg_shell *create_xdg_shell(struct wl_display *display) {
    if constexpr (std::is_invocable_v<decltype(&wlr_xdg_shell_create), struct wl_display *, uint32_t>) {
        return wlr_xdg_shell_create(display, DEFAULT_XDG_VERSION);
    } else {
        return wlr_xdg_shell_create(display);
    }
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
    setenv("XDG_RUNTIME_DIR", fallback.c_str(), true);
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

void xdg_surface_map(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, map);
    view->mapped = true;
    view->opacity = 0.0f;

    auto animation = std::make_unique<Animation>();
    animation->start(0.0f, 1.0f, SwissDesign::ANIMATION_DURATION, [view](float value) {
        view->opacity = value;
    });
    push_animation(view->server, std::move(animation));

    static int window_count = 0;
    view->x = (window_count % 2) * 640;
    view->y = (window_count / 2) * 480 + SwissDesign::PANEL_HEIGHT;
    window_count++;

    wlr_log(WLR_INFO, "Surface mapped at %d,%d", view->x, view->y);
}

void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->link);
    free(view);
}

void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    (void)listener;
    (void)data;
}

void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    (void)listener;
    (void)data;
}
} // namespace

static ArolloaServer *g_server = nullptr;

static void handle_signal(int sig) {
    (void)sig;
    if (g_server && g_server->wl_display) {
        wl_display_terminate(g_server->wl_display);
    }
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_xdg_surface);
    auto *xdg_surface = static_cast<struct wlr_xdg_surface *>(data);

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    ArolloaView *view = static_cast<ArolloaView *>(calloc(1, sizeof(ArolloaView)));
    if (!view) {
        return;
    }

    view->server = server;
    view->xdg_surface = xdg_surface;
    view->opacity = 1.0f;

    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->surface->events.map, &view->map);

    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);

    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    if (xdg_surface->toplevel) {
        view->request_move.notify = xdg_toplevel_request_move;
        wl_signal_add(&xdg_surface->toplevel->events.request_move, &view->request_move);

        view->request_resize.notify = xdg_toplevel_request_resize;
        wl_signal_add(&xdg_surface->toplevel->events.request_resize, &view->request_resize);
    }

    wl_list_insert(&server->views, &view->link);
}

void server_init(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->initialized = false;
    ensure_runtime_dir();
    setup_debug_environment(server);

    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "Failed to create Wayland display");
        return;
    }

    server->backend = autocreate_backend(server->wl_display);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "Failed to create wlroots backend");
        return;
    }

    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create renderer");
        return;
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->compositor = create_compositor(server->wl_display, server->renderer);
    if (!server->compositor) {
        wlr_log(WLR_ERROR, "Failed to create compositor global");
        return;
    }

    server->xdg_shell = create_xdg_shell(server->wl_display);
    if (!server->xdg_shell) {
        wlr_log(WLR_ERROR, "Failed to create xdg-shell global");
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
        return;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        return;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Running Arolloa on WAYLAND_DISPLAY=%s%s", socket,
            server->debug_mode ? " (debug nested mode)" : "");

    schedule_startup_animation(server);
    server->initialized = true;
}

void server_run(ArolloaServer *server) {
    if (!server || !server->initialized) {
        return;
    }
    g_server = server;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    wl_display_run(server->wl_display);
}

void server_destroy(ArolloaServer *server) {
    if (!server) {
        return;
    }

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

    if (server->backend) {
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;
    }

    if (server->cairo_ctx) {
        cairo_destroy(server->cairo_ctx);
        server->cairo_ctx = nullptr;
    }
    if (server->ui_surface) {
        cairo_surface_destroy(server->ui_surface);
        server->ui_surface = nullptr;
    }
    if (server->pango_layout) {
        g_object_unref(server->pango_layout);
        server->pango_layout = nullptr;
    }

    if (server->wl_display) {
        wl_display_destroy_clients(server->wl_display);
        wl_display_destroy(server->wl_display);
        server->wl_display = nullptr;
    }

    server->animations.clear();
    server->initialized = false;
}
