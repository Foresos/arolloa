#include "../../include/arolloa.h"
#include <wlr/version.h>

#include <csignal>

namespace {
ArolloaServer *g_server = nullptr;

void handle_signal(int sig) {
    (void)sig;
    if (g_server && g_server->wl_display) {
        wl_display_terminate(g_server->wl_display);
    }
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

void destroy_decoration_manager(struct wlr_xdg_decoration_manager_v1 *manager) {
    if (!manager) {
        return;
    }

#if !defined(WLR_VERSION_NUM) || WLR_VERSION_NUM < ((0 << 16) | (18 << 8) | 0)
    wlr_xdg_decoration_manager_v1_destroy(manager);
#else
    (void)manager;
#endif
}

void destroy_xdg_shell(struct wlr_xdg_shell *shell) {
    if (!shell) {
        return;
    }

#if !defined(WLR_VERSION_NUM) || WLR_VERSION_NUM < ((0 << 16) | (18 << 8) | 0)
    wlr_xdg_shell_destroy(shell);
#else
    (void)shell;
#endif
}

void destroy_compositor(struct wlr_compositor *compositor) {
    if (!compositor) {
        return;
    }

#if !defined(WLR_VERSION_NUM) || WLR_VERSION_NUM < ((0 << 16) | (18 << 8) | 0)
    wlr_compositor_destroy(compositor);
#else
    (void)compositor;
#endif
}
} // namespace

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

    if (server->new_decoration.link.next) {
        wl_list_remove(&server->new_decoration.link);
        server->new_decoration.link.next = nullptr;
        server->new_decoration.link.prev = nullptr;
    }

    ArolloaView *view = nullptr;
    ArolloaView *tmp = nullptr;
    wl_list_for_each_safe(view, tmp, &server->views, link) {
        wl_list_remove(&view->link);
        delete view;
    }
    wl_list_init(&server->views);

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

    if (server->allocator) {
        wlr_allocator_destroy(server->allocator);
        server->allocator = nullptr;
    }

    if (server->decoration_manager) {
        destroy_decoration_manager(server->decoration_manager);
        server->decoration_manager = nullptr;
    }

    if (server->xdg_shell) {
        destroy_xdg_shell(server->xdg_shell);
        server->xdg_shell = nullptr;
    }

    if (server->compositor) {
        destroy_compositor(server->compositor);
        server->compositor = nullptr;
    }

    if (server->renderer) {
        wlr_renderer_destroy(server->renderer);
        server->renderer = nullptr;
    }

    if (server->backend) {
        wlr_backend_destroy(server->backend);
        server->backend = nullptr;
    }

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    if (server->session) {
        wlr_session_destroy(server->session);
        server->session = nullptr;
    }
#endif

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

    destroy_display(server);

    server->animations.clear();
    server->focused_view = nullptr;
    server->initialized = false;
}
