#include "../../include/arolloa.h"

#include <memory>
#include <new>

namespace {
void xdg_surface_map(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, map);
    view->mapped = true;
    view->opacity = 0.0f;

    struct wlr_box geometry = {};
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geometry);
    view->width = geometry.width > 0 ? geometry.width : 640;
    view->height = geometry.height > 0 ? geometry.height : 480;
    view->is_fullscreen = view->toplevel && view->toplevel->current.fullscreen;
    view->is_maximized = view->toplevel && view->toplevel->current.maximized;
    view->is_minimized = false;

    if (view->toplevel) {
        view->title = view->toplevel->title ? view->toplevel->title : "";
        view->app_id = view->toplevel->app_id ? view->toplevel->app_id : "";
    }

    arrange_views(view->server);
    focus_view(view->server, view);

    auto animation = std::make_unique<Animation>();
    animation->start(0.0f, 1.0f, SwissDesign::ANIMATION_DURATION, [view](float value) {
        view->opacity = value;
    });
    push_animation(view->server, std::move(animation));

    wlr_log(WLR_INFO, "Surface mapped (%s) at %d,%d", view->title.c_str(), view->x, view->y);
}

void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    view->is_fullscreen = false;
    view->is_maximized = false;
    view->is_minimized = false;

    if (view->server->focused_view == view) {
        focus_view(view->server, nullptr);
    }

    arrange_views(view->server);
}

void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->link);

    if (view->toplevel) {
        wl_list_remove(&view->request_move.link);
        wl_list_remove(&view->request_resize.link);
        wl_list_remove(&view->request_maximize.link);
        wl_list_remove(&view->request_fullscreen.link);
        wl_list_remove(&view->request_minimize.link);
        wl_list_remove(&view->request_show_window_menu.link);
        wl_list_remove(&view->set_title.link);
        wl_list_remove(&view->set_app_id.link);
        wl_list_remove(&view->set_parent.link);
    }

    if (view->server->focused_view == view) {
        view->server->focused_view = nullptr;
    }

    delete view;
}

void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_move);
    wlr_log(WLR_DEBUG, "Move requested for %s", view->title.c_str());
}

void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_resize);
    view->is_maximized = false;
    wlr_log(WLR_DEBUG, "Resize requested for %s", view->title.c_str());
}

void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_maximize);
    if (view->toplevel) {
        view->is_maximized = view->toplevel->requested.maximized;
    }
    arrange_views(view->server);
}

void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_fullscreen);
    if (view->toplevel) {
        view->is_fullscreen = view->toplevel->requested.fullscreen;
    }
    arrange_views(view->server);
}

void xdg_toplevel_request_minimize(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_minimize);
    if (view->toplevel) {
        view->is_minimized = view->toplevel->requested.minimized;
        if (view->is_minimized) {
            wlr_xdg_toplevel_set_activated(view->toplevel, false);
        }
    }
    arrange_views(view->server);
}

void xdg_toplevel_request_show_window_menu(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, request_show_window_menu);
    wlr_log(WLR_DEBUG, "Window menu requested for %s", view->title.c_str());
}

void xdg_toplevel_set_title(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, set_title);
    if (view->toplevel) {
        view->title = view->toplevel->title ? view->toplevel->title : "";
    }
}

void xdg_toplevel_set_app_id(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, set_app_id);
    if (view->toplevel) {
        view->app_id = view->toplevel->app_id ? view->toplevel->app_id : "";
    }
}

void xdg_toplevel_set_parent(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, set_parent);
    (void)view;
}
} // namespace

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_xdg_surface);
    auto *xdg_surface = static_cast<struct wlr_xdg_surface *>(data);

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    auto *view = new (std::nothrow) ArolloaView();
    if (!view) {
        return;
    }

    view->server = server;
    view->xdg_surface = xdg_surface;
    view->toplevel = xdg_surface->toplevel;
    view->mapped = false;
    view->opacity = 1.0f;
    view->x = 0;
    view->y = 0;
    view->width = 0;
    view->height = 0;
    view->is_fullscreen = false;
    view->is_maximized = false;
    view->is_minimized = false;
    wl_list_init(&view->link);

    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->surface->events.map, &view->map);

    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);

    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    if (view->toplevel) {
        wlr_xdg_toplevel_set_wm_capabilities(view->toplevel,
            WLR_XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU |
            WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE |
            WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN |
            WLR_XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE);

        view->request_move.notify = xdg_toplevel_request_move;
        wl_signal_add(&view->toplevel->events.request_move, &view->request_move);

        view->request_resize.notify = xdg_toplevel_request_resize;
        wl_signal_add(&view->toplevel->events.request_resize, &view->request_resize);

        view->request_maximize.notify = xdg_toplevel_request_maximize;
        wl_signal_add(&view->toplevel->events.request_maximize, &view->request_maximize);

        view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
        wl_signal_add(&view->toplevel->events.request_fullscreen, &view->request_fullscreen);

        view->request_minimize.notify = xdg_toplevel_request_minimize;
        wl_signal_add(&view->toplevel->events.request_minimize, &view->request_minimize);

        view->request_show_window_menu.notify = xdg_toplevel_request_show_window_menu;
        wl_signal_add(&view->toplevel->events.request_show_window_menu, &view->request_show_window_menu);

        view->set_title.notify = xdg_toplevel_set_title;
        wl_signal_add(&view->toplevel->events.set_title, &view->set_title);

        view->set_app_id.notify = xdg_toplevel_set_app_id;
        wl_signal_add(&view->toplevel->events.set_app_id, &view->set_app_id);

        view->set_parent.notify = xdg_toplevel_set_parent;
        wl_signal_add(&view->toplevel->events.set_parent, &view->set_parent);
    }

    wl_list_insert(&server->views, &view->link);
}
