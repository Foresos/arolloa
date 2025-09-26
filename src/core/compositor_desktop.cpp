#include "../../include/arolloa.h"

#include <algorithm>

namespace {
struct OutputMetrics {
    int width{1280};
    int height{720};
};

OutputMetrics query_output_metrics(ArolloaServer *server) {
    OutputMetrics metrics;
    if (!server || !server->output_layout) {
        return metrics;
    }

    struct wlr_box box = {};
    wlr_output_layout_get_box(server->output_layout, nullptr, &box);
    if (box.width > 0) {
        metrics.width = box.width;
    }
    if (box.height > 0) {
        metrics.height = box.height;
    }
    return metrics;
}

void apply_view_bounds(ArolloaView *view, int width, int height) {
    if (!view || !view->toplevel) {
        return;
    }

    const int32_t clamped_width = std::max(320, width);
    const int32_t clamped_height = std::max(240, height);

    wlr_xdg_toplevel_set_fullscreen(view->toplevel, view->is_fullscreen);
    if (!view->is_fullscreen) {
        wlr_xdg_toplevel_set_maximized(view->toplevel, view->is_maximized);
        wlr_xdg_toplevel_set_size(view->toplevel, clamped_width, clamped_height);
    }
}

void layout_grid(ArolloaServer *server, const OutputMetrics &metrics) {
    if (!server) {
        return;
    }

    const int gap = std::max(0, get_config_int("layout.gap", SwissDesign::WINDOW_GAP));
    const int panel_height = std::max(0, get_config_int("appearance.panel_height", SwissDesign::PANEL_HEIGHT));
    const int usable_width = std::max(1, metrics.width - (gap * 2));
    const int usable_height = std::max(1, metrics.height - panel_height - (gap * 2));

    int columns = 2;
    if (usable_width < 900) {
        columns = 1;
    } else if (usable_width > 1600) {
        columns = 3;
    }

    int column_width = std::max(1, usable_width / columns);
    int x = gap;
    int y = panel_height + gap;
    int row_height = 0;

    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        if (!view->mapped || view->is_fullscreen || view->is_minimized) {
            continue;
        }

        struct wlr_box geometry = {};
        wlr_xdg_surface_get_geometry(view->xdg_surface, &geometry);

        const int column_span = std::max(1, column_width - gap);
        const int width = std::min(column_span, geometry.width > 0 ? geometry.width : column_width);
        const int height = std::min(usable_height, geometry.height > 0 ? geometry.height : std::max(usable_height / 2, 240));

        if (x + width > metrics.width - gap) {
            x = gap;
            y += row_height + gap;
            row_height = 0;
        }

        view->x = x;
        view->y = y;
        view->width = width;
        view->height = height;
        view->is_maximized = false;

        apply_view_bounds(view, width, height);

        x += width + gap;
        row_height = std::max(row_height, height);
    }
}

void layout_floating(ArolloaServer *server, const OutputMetrics &metrics) {
    if (!server) {
        return;
    }

    const int gap = std::max(0, get_config_int("layout.gap", SwissDesign::WINDOW_GAP));
    const int panel_height = std::max(0, get_config_int("appearance.panel_height", SwissDesign::PANEL_HEIGHT));
    int cursor_x = gap;
    int cursor_y = panel_height + gap;

    const int max_width = std::max(320, metrics.width - (gap * 2));
    const int max_height = std::max(200, metrics.height - panel_height - (gap * 2));

    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        if (!view->mapped || view->is_fullscreen || view->is_minimized) {
            continue;
        }

        struct wlr_box geometry = {};
        wlr_xdg_surface_get_geometry(view->xdg_surface, &geometry);

        const int width = std::clamp(geometry.width > 0 ? geometry.width : max_width / 2, 320, max_width);
        const int height = std::clamp(geometry.height > 0 ? geometry.height : max_height / 2, 200, max_height);

        if (cursor_x + width > metrics.width - gap) {
            cursor_x = gap;
            cursor_y += height + gap;
        }

        view->x = cursor_x;
        view->y = cursor_y;
        view->width = width;
        view->height = height;
        view->is_maximized = false;

        apply_view_bounds(view, width, height);

        cursor_x += width + gap;
    }
}

void layout_fullscreen(ArolloaServer *server, const OutputMetrics &metrics) {
    if (!server) {
        return;
    }

    const int panel_height = std::max(0, get_config_int("appearance.panel_height", SwissDesign::PANEL_HEIGHT));
    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        if (!view->mapped || !view->is_fullscreen) {
            continue;
        }
        view->x = 0;
        view->y = panel_height;
        view->width = metrics.width;
        view->height = metrics.height - panel_height;
        apply_view_bounds(view, view->width, view->height);
    }
}
} // namespace

void arrange_views(ArolloaServer *server) {
    if (!server) {
        return;
    }

    const OutputMetrics metrics = query_output_metrics(server);

    switch (server->layout_mode) {
        case WindowLayout::GRID:
        case WindowLayout::ASYMMETRICAL:
            layout_grid(server, metrics);
            break;
        case WindowLayout::FLOATING:
            layout_floating(server, metrics);
            break;
    }

    layout_fullscreen(server, metrics);
}

void focus_view(ArolloaServer *server, ArolloaView *view) {
    if (!server || server->seat == nullptr) {
        return;
    }

    if (server->focused_view == view) {
        return;
    }

    if (!view || !view->mapped) {
        wlr_seat_keyboard_notify_clear_focus(server->seat);
        server->focused_view = nullptr;
        return;
    }

    struct wlr_surface *surface = view->xdg_surface ? view->xdg_surface->surface : nullptr;
    if (!surface) {
        return;
    }

    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
    const uint32_t *keycodes = nullptr;
    size_t num_keycodes = 0;
    const struct wlr_keyboard_modifiers *modifiers = nullptr;

    if (keyboard) {
        keycodes = keyboard->keycodes;
        num_keycodes = keyboard->num_keycodes;
        modifiers = &keyboard->modifiers;
    }

    wlr_seat_keyboard_notify_enter(server->seat, surface, keycodes, num_keycodes, modifiers);
    if (view->toplevel) {
        wlr_xdg_toplevel_set_activated(view->toplevel, true);
    }

    if (server->focused_view && server->focused_view != view && server->focused_view->toplevel) {
        wlr_xdg_toplevel_set_activated(server->focused_view->toplevel, false);
    }

    wl_list_remove(&view->link);
    wl_list_insert(&server->views, &view->link);
    server->focused_view = view;
}
