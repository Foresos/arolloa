#include "../../include/arolloa.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include <wlr/render/pass.h>
#include <drm_fourcc.h>

namespace {
float linear_interpolate(float from, float to, float t) {
    return from + (to - from) * t;
}

constexpr double kPi = 3.14159265358979323846;
constexpr double kHalfPi = kPi / 2.0;

struct timespec get_monotonic_time() {
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

SwissDesign::Color lerp_color(const SwissDesign::Color &a, const SwissDesign::Color &b, float t) {
    return SwissDesign::Color(
        linear_interpolate(a.r, b.r, t),
        linear_interpolate(a.g, b.g, t),
        linear_interpolate(a.b, b.b, t),
        linear_interpolate(a.a, b.a, t));
}

void set_source_color(cairo_t *cairo, const SwissDesign::Color &color, float opacity) {
    cairo_set_source_rgba(cairo, color.r, color.g, color.b, color.a * opacity);
}

int count_mapped_views(const ArolloaServer *server) {
    int count = 0;
    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        if (view->mapped) {
            ++count;
        }
    }
    return count;
}

std::string format_debug_info(const ArolloaServer *server) {
    std::ostringstream ss;
    ss << (server->nested_backend_active ? "Nested" : "Direct");
    ss << " | Views " << count_mapped_views(server);
    ss << " | Cursor " << static_cast<int>(server->cursor_x) << "," << static_cast<int>(server->cursor_y);
    ss << " | Animations " << (server->animations.empty() ? "idle" : std::to_string(server->animations.size()));
    return ss.str();
}

void apply_font(PangoLayout *layout, const std::string &font, int size_pt) {
    PangoFontDescription *desc = pango_font_description_from_string((font + " " + std::to_string(size_pt)).c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
}

void draw_text(cairo_t *cr, PangoLayout *layout, const std::string &text, double x, double y,
               const SwissDesign::Color &color, float opacity, PangoAlignment alignment = PANGO_ALIGN_LEFT) {
    if (!layout) {
        return;
    }
    cairo_save(cr);
    cairo_move_to(cr, x, y);
    pango_layout_set_alignment(layout, alignment);
    pango_layout_set_width(layout, -1);
    pango_layout_set_text(layout, text.c_str(), -1);
    set_source_color(cr, color, opacity);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
}

void draw_panel_apps(cairo_t *cr, const ArolloaServer *server, float opacity) {
    const double icon_size = 28.0;
    const double spacing = 12.0;
    double x = FOREST_PANEL_MENU_WIDTH + spacing;
    const double y = (SwissDesign::PANEL_HEIGHT - icon_size) / 2.0;

    for (const auto &app : server->ui_state.panel_apps) {
        cairo_save(cr);
        cairo_rectangle(cr, x, y, icon_size, icon_size);
        set_source_color(cr, SwissDesign::Forest::MOSS_ACCENT, opacity * 0.85f);
        cairo_fill(cr);
        cairo_restore(cr);

        if (server->pango_layout) {
            apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
            draw_text(cr, server->pango_layout, app.icon_label, x + 6.0, y + 6.0,
                      SwissDesign::Forest::BARK, opacity);
        }

        x += icon_size + spacing;
    }
}

void draw_tray_icons(cairo_t *cr, const ArolloaServer *server, int width, float opacity) {
    double x = static_cast<double>(width) - 16.0;
    const double icon_size = 22.0;

    for (auto it = server->ui_state.tray_icons.rbegin(); it != server->ui_state.tray_icons.rend(); ++it) {
        x -= icon_size;
        cairo_save(cr);
        cairo_arc(cr, x + icon_size / 2.0, SwissDesign::PANEL_HEIGHT / 2.0, icon_size / 2.5, 0, 2 * kPi);
        set_source_color(cr, it->color, opacity * 0.9f);
        cairo_fill(cr);
        cairo_restore(cr);

        if (server->pango_layout) {
            apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 8);
            draw_text(cr, server->pango_layout, it->label, x - 2.0, SwissDesign::PANEL_HEIGHT / 2.0 - 6.0,
                      SwissDesign::WHITE, opacity, PANGO_ALIGN_LEFT);
        }

        x -= 18.0;
    }
}

void draw_panel_branding(cairo_t *cr, const ArolloaServer *server, float opacity) {
    if (!server->pango_layout) {
        return;
    }
    apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 14);
    draw_text(cr, server->pango_layout, "Arolloa", 18.0, SwissDesign::PANEL_HEIGHT / 2.0 - 8.0,
              SwissDesign::Forest::SUNLIGHT, opacity);
    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
    draw_text(cr, server->pango_layout, "Launcher", FOREST_PANEL_MENU_WIDTH - 58.0, SwissDesign::PANEL_HEIGHT / 2.0 + 2.0,
              SwissDesign::Forest::CANOPY_LIGHT, opacity, PANGO_ALIGN_RIGHT);
}

void draw_panel_debug(cairo_t *cr, const ArolloaServer *server, int width, float opacity) {
    if (!server->pango_layout) {
        return;
    }
    apply_font(server->pango_layout, SwissDesign::MONO_FONT, 9);
    draw_text(cr, server->pango_layout, format_debug_info(server), width * 0.35,
              SwissDesign::PANEL_HEIGHT / 2.0 - 6.0, SwissDesign::Forest::SUNLIGHT, opacity);
}

void draw_rounded_rect(cairo_t *cr, double x, double y, double width, double height, double radius) {
    cairo_new_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -kHalfPi, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, kHalfPi);
    cairo_arc(cr, x + radius, y + height - radius, radius, kHalfPi, kPi);
    cairo_arc(cr, x + radius, y + radius, radius, kPi, 3 * kHalfPi);
    cairo_close_path(cr);
}

void render_launcher_overlay(cairo_t *cr, ArolloaServer *server, int width, int height, float opacity) {
    if (!server->ui_state.launcher_visible || !server->pango_layout) {
        return;
    }

    cairo_save(cr);
    set_source_color(cr, SwissDesign::Forest::CANOPY_DARK, 0.55f * opacity);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    const double panel_width = FOREST_LAUNCHER_WIDTH;
    const double panel_height = std::min<double>(height * 0.6,
        std::max<double>(SwissDesign::PANEL_HEIGHT * 4.0,
            server->ui_state.launcher_entries.size() * FOREST_LAUNCHER_ENTRY_HEIGHT + 120.0));
    const double start_x = (width - panel_width) / 2.0;
    const double start_y = (height - panel_height) / 2.0;

    draw_rounded_rect(cr, start_x, start_y, panel_width, panel_height, 18.0);
    set_source_color(cr, SwissDesign::Forest::CANOPY_LIGHT, 0.95f * opacity);
    cairo_fill(cr);

    apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 16);
    draw_text(cr, server->pango_layout, "Forest Launcher", start_x + 32.0, start_y + 26.0,
              SwissDesign::Forest::BARK, opacity);

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
    draw_text(cr, server->pango_layout, "Launch curated system tools and applications",
              start_x + 32.0, start_y + 54.0, SwissDesign::Forest::BARK, opacity * 0.9f);

    double entry_y = start_y + 88.0;
    std::size_t index = 0;
    for (const auto &entry : server->ui_state.launcher_entries) {
        const bool highlighted = index == server->ui_state.highlighted_index;
        cairo_save(cr);
        draw_rounded_rect(cr, start_x + 24.0, entry_y, panel_width - 48.0, FOREST_LAUNCHER_ENTRY_HEIGHT - 8.0, 12.0);
        if (highlighted) {
            set_source_color(cr, SwissDesign::Forest::MOSS_ACCENT, 0.65f * opacity);
        } else {
            set_source_color(cr, SwissDesign::Forest::CANOPY_MID, 0.35f * opacity);
        }
        cairo_fill(cr);
        cairo_restore(cr);

        apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 13);
        draw_text(cr, server->pango_layout, entry.name, start_x + 48.0, entry_y + 12.0,
                  SwissDesign::WHITE, opacity);

        apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
        draw_text(cr, server->pango_layout, entry.description, start_x + 48.0, entry_y + 32.0,
                  SwissDesign::Forest::SUNLIGHT, opacity * 0.9f);

        apply_font(server->pango_layout, SwissDesign::MONO_FONT, 8);
        draw_text(cr, server->pango_layout, entry.category, start_x + panel_width - 120.0,
                  entry_y + 14.0, SwissDesign::Forest::BARK, opacity, PANGO_ALIGN_RIGHT);

        entry_y += FOREST_LAUNCHER_ENTRY_HEIGHT;
        ++index;
    }

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
    draw_text(cr, server->pango_layout, "Hint: Super + Space toggles the launcher",
              start_x + 32.0, start_y + panel_height - 42.0, SwissDesign::Forest::SUNLIGHT, opacity * 0.8f);

    cairo_restore(cr);
}

} // namespace

void render_swiss_panel(cairo_t *cairo, int width, int height, float opacity, const ArolloaServer *server) {
    (void)height;
    cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, 0, SwissDesign::PANEL_HEIGHT);
    auto top_color = SwissDesign::Forest::CANOPY_DARK;
    auto bottom_color = SwissDesign::Forest::CANOPY_LIGHT;
    cairo_pattern_add_color_stop_rgba(pattern, 0.0, top_color.r, top_color.g, top_color.b, opacity);
    cairo_pattern_add_color_stop_rgba(pattern, 1.0, bottom_color.r, bottom_color.g, bottom_color.b, opacity);

    cairo_save(cairo);
    cairo_rectangle(cairo, 0, 0, width, SwissDesign::PANEL_HEIGHT);
    cairo_set_source(cairo, pattern);
    cairo_fill(cairo);
    cairo_restore(cairo);
    cairo_pattern_destroy(pattern);

    cairo_save(cairo);
    cairo_rectangle(cairo, 0, SwissDesign::PANEL_HEIGHT - SwissDesign::BORDER_WIDTH, width, SwissDesign::BORDER_WIDTH);
    set_source_color(cairo, SwissDesign::Forest::MOSS_ACCENT, opacity);
    cairo_fill(cairo);
    cairo_restore(cairo);

    draw_panel_branding(cairo, server, opacity);
    draw_panel_apps(cairo, server, opacity);
    draw_tray_icons(cairo, server, width, opacity);
    draw_panel_debug(cairo, server, width, opacity);
}

void render_swiss_window(cairo_t *cairo, ArolloaView *view, float global_opacity) {
    if (!view->mapped) {
        return;
    }

    const float opacity = view->opacity * global_opacity;
    set_source_color(cairo, SwissDesign::Forest::CANOPY_LIGHT, opacity * 0.85f);
    cairo_rectangle(cairo, view->x, view->y, 400, 300);
    cairo_fill(cairo);

    cairo_set_line_width(cairo, SwissDesign::BORDER_WIDTH);
    set_source_color(cairo, SwissDesign::Forest::BARK, opacity);
    cairo_rectangle(cairo, view->x, view->y, 400, 300);
    cairo_stroke(cairo);
}

void render_swiss_ui(ArolloaServer *server, ArolloaOutput *output) {
    int width = 0;
    int height = 0;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    int surface_width = cairo_image_surface_get_width(server->ui_surface);
    int surface_height = cairo_image_surface_get_height(server->ui_surface);

    if (surface_width != width || surface_height != height) {
        cairo_destroy(server->cairo_ctx);
        cairo_surface_destroy(server->ui_surface);
        server->ui_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        server->cairo_ctx = cairo_create(server->ui_surface);
    }

    cairo_save(server->cairo_ctx);
    cairo_set_operator(server->cairo_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(server->cairo_ctx, 0, 0, 0, 0);
    cairo_paint(server->cairo_ctx);
    cairo_restore(server->cairo_ctx);

    const float opacity = std::clamp(server->startup_opacity, 0.0f, 1.0f);
    render_swiss_panel(server->cairo_ctx, width, height, opacity, server);
    render_launcher_overlay(server->cairo_ctx, server, width, height, opacity);

    cairo_surface_flush(server->ui_surface);
}

void initialize_forest_ui(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->ui_state.panel_apps = {
        {"Files", "thunar", "Fs"},
        {"Terminal", "foot", "Tm"},
        {"Browser", "firefox", "Web"}
    };

    server->ui_state.tray_icons = {
        {"NET", "Network status", SwissDesign::Forest::SUNLIGHT},
        {"VOL", "Audio level", SwissDesign::Forest::MOSS_ACCENT},
        {"PWR", "Power status", SwissDesign::Forest::BARK}
    };

    server->ui_state.launcher_entries = {
        {"Forest Terminal", "foot", "A minimalist Wayland terminal optimized for clarity.", "System"},
        {"Web Browser", "firefox", "Launch a modern browser with privacy enhancements.", "Internet"},
        {"File Manager", "thunar", "Browse the Swiss filesystem with precision.", "Productivity"},
        {"Settings", "./build/arolloa-settings", "Configure the compositor without GTK dependencies.", "Control"},
        {"Flatpak Manager", "flatpak run com.valvesoftware.Steam", "Access packaged applications and games.", "Apps"},
        {"System Monitor", "gnome-system-monitor", "Inspect processes and resource utilization.", "Diagnostics"}
    };

    server->ui_state.launcher_visible = false;
    server->ui_state.highlighted_index = 0;
    server->ui_state.last_interaction = std::chrono::steady_clock::now();
}

void output_frame(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaOutput *output = wl_container_of(listener, output, frame);
    ArolloaServer *server = output->server;

    const struct timespec now = get_monotonic_time();

    int width = 0;
    int height = 0;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    const float fade = std::clamp(server->startup_opacity, 0.0f, 1.0f);

    struct wlr_output_state state;
    wlr_output_state_init(&state);

    struct wlr_render_pass *render_pass = wlr_output_begin_render_pass(output->wlr_output, &state, nullptr, nullptr);
    if (!render_pass) {
        wlr_output_state_finish(&state);
        return;
    }

    const struct wlr_box top_box = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = height / 2,
    };
    struct wlr_render_rect_options top_rect = {};
    top_rect.box = top_box;
    auto top_color = lerp_color(SwissDesign::Forest::CANOPY_DARK, SwissDesign::Forest::CANOPY_MID, fade);
    top_rect.color = {.r = top_color.r * fade, .g = top_color.g * fade, .b = top_color.b * fade, .a = fade};
    wlr_render_pass_add_rect(render_pass, &top_rect);

    const struct wlr_box bottom_box = {
        .x = 0,
        .y = height / 2,
        .width = width,
        .height = height - height / 2,
    };
    struct wlr_render_rect_options bottom_rect = {};
    bottom_rect.box = bottom_box;
    auto bottom_color = lerp_color(SwissDesign::Forest::CANOPY_MID, SwissDesign::Forest::CANOPY_LIGHT, fade);
    bottom_rect.color = {.r = bottom_color.r * fade, .g = bottom_color.g * fade, .b = bottom_color.b * fade, .a = fade};
    wlr_render_pass_add_rect(render_pass, &bottom_rect);

    const struct wlr_box panel_box = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = SwissDesign::PANEL_HEIGHT
    };

    struct wlr_render_rect_options panel_rect = {};
    panel_rect.box = panel_box;
    auto panel_color = lerp_color(SwissDesign::Forest::CANOPY_DARK, SwissDesign::Forest::CANOPY_LIGHT, 0.35f);
    panel_rect.color = {.r = panel_color.r * fade, .g = panel_color.g * fade, .b = panel_color.b * fade, .a = fade};
    wlr_render_pass_add_rect(render_pass, &panel_rect);

    animation_tick(server);

    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        if (!view->mapped) {
            continue;
        }
        struct wlr_surface *surface = view->xdg_surface->surface;
        struct wlr_texture *texture = wlr_surface_get_texture(surface);
        if (!texture) {
            continue;
        }

        const float alpha = std::clamp(view->opacity * fade, 0.0f, 1.0f);
        if (alpha <= 0.0f) {
            continue;
        }

        const struct wlr_box box = {
            .x = view->x,
            .y = view->y,
            .width = surface->current.width,
            .height = surface->current.height
        };

        struct wlr_render_texture_options texture_options = {};
        texture_options.texture = texture;
        texture_options.dst_box = box;
        if (alpha < 1.0f) {
            texture_options.alpha = &alpha;
        }
        wlr_render_pass_add_texture(render_pass, &texture_options);

        wlr_surface_send_frame_done(surface, &now);
    }

    render_swiss_ui(server, output);
    struct wlr_texture *ui_texture = wlr_texture_from_pixels(server->renderer, DRM_FORMAT_ARGB8888,
        cairo_image_surface_get_stride(server->ui_surface), width, height,
        cairo_image_surface_get_data(server->ui_surface));
    if (ui_texture) {
        struct wlr_render_texture_options ui_options = {};
        ui_options.texture = ui_texture;
        ui_options.dst_box = {
            .x = 0,
            .y = 0,
            .width = width,
            .height = height,
        };
        wlr_render_pass_add_texture(render_pass, &ui_options);
        wlr_texture_destroy(ui_texture);
    }

    if (!wlr_render_pass_submit(render_pass)) {
        wlr_output_state_finish(&state);
        return;
    }

    if (!wlr_output_commit_state(output->wlr_output, &state)) {
        wlr_output_state_finish(&state);
        return;
    }

    wlr_output_state_finish(&state);
}

namespace {
void remove_output_listeners(ArolloaOutput *output) {
    if (!output) {
        return;
    }

    wl_list_remove(&output->frame.link);
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    wl_list_remove(&output->request_state.link);
#endif
    wl_list_remove(&output->link);
}
} // namespace

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
static void output_request_state(struct wl_listener *listener, void *data) {
    ArolloaOutput *output = wl_container_of(listener, output, request_state);
    const auto *event = static_cast<const struct wlr_output_event_request_state *>(data);
    if (!event || !event->state) {
        return;
    }

    if (!wlr_output_commit_state(output->wlr_output, event->state)) {
        wlr_log(WLR_ERROR, "Failed to apply requested output state");
    }
}
#endif

void server_new_output(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_output);
    auto *wlr_output = static_cast<struct wlr_output *>(data);

    if (!wlr_output_init_render(wlr_output, server->allocator, server->renderer)) {
        wlr_log(WLR_ERROR, "Failed to initialize output render resources");
        return;
    }

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        if (mode) {
            wlr_output_state_set_mode(&state, mode);
        }
    }

    if (!wlr_output_commit_state(wlr_output, &state)) {
        wlr_log(WLR_ERROR, "Failed to commit initial output state");
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);

    ArolloaOutput *output = static_cast<ArolloaOutput *>(calloc(1, sizeof(ArolloaOutput)));
    if (!output) {
        return;
    }

    output->wlr_output = wlr_output;
    output->server = server;
    output->last_frame = get_monotonic_time();

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (17 << 8) | 0)
    output->request_state.notify = output_request_state;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);
#endif

    output->destroy.notify = [](struct wl_listener *listener, void *data) {
        (void)data;
        ArolloaOutput *output = wl_container_of(listener, output, destroy);
        remove_output_listeners(output);
        free(output);
    };
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    wlr_log(WLR_INFO, "Registered output '%s'", wlr_output->name);
}
