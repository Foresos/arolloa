#include "../../include/arolloa.h"

#include <csignal>

namespace {
ArolloaServer *g_server = nullptr;

void handle_signal(int sig) {
    (void)sig;
    if (g_server && g_server->wl_display) {
        wl_display_terminate(g_server->wl_display);
    }
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
