#include "../../include/arolloa.h"

#include <memory>

namespace {
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
