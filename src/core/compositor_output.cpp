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

SwissDesign::Color color_from_hex(const std::string &hex, const SwissDesign::Color &fallback) {
    if (hex.size() != 7 || hex.front() != '#') {
        return fallback;
    }

    auto parse_channel = [](char high, char low) -> float {
        auto hex_to_int = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return 10 + (c - 'a');
            }
            if (c >= 'A' && c <= 'F') {
                return 10 + (c - 'A');
            }
            return 0;
        };
        const int value = (hex_to_int(high) << 4) | hex_to_int(low);
        return std::clamp(value / 255.0f, 0.0f, 1.0f);
    };

    return SwissDesign::Color(
        parse_channel(hex[1], hex[2]),
        parse_channel(hex[3], hex[4]),
        parse_channel(hex[5], hex[6]),
        1.0f);
}

SwissDesign::Color lighten(const SwissDesign::Color &color, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    return lerp_color(color, SwissDesign::WHITE, amount);
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

void draw_text_center(cairo_t *cr, PangoLayout *layout, const std::string &text, double x, double y,
                      const SwissDesign::Color &color, float opacity) {
    if (!layout) {
        return;
    }
    cairo_save(cr);
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    pango_layout_set_width(layout, -1);
    pango_layout_set_text(layout, text.c_str(), -1);
    int text_width = 0;
    int text_height = 0;
    pango_layout_get_pixel_size(layout, &text_width, &text_height);
    cairo_move_to(cr, x - text_width / 2.0, y);
    set_source_color(cr, color, opacity);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
}

void draw_panel_apps(cairo_t *cr, const ArolloaServer *server, float opacity) {
    const double icon_size = 28.0;
    const double spacing = 18.0;
    double x = FOREST_PANEL_MENU_WIDTH + spacing;
    const double y = (SwissDesign::PANEL_HEIGHT - icon_size) / 2.0;

    for (std::size_t index = 0; index < server->ui_state.panel_apps.size(); ++index) {
        const auto &app = server->ui_state.panel_apps[index];
        const bool hovered = static_cast<int>(index) == server->ui_state.hovered_panel_index;
        const float progress = hovered ? server->ui_state.panel_hover_progress : 0.0f;
        const float halo_opacity = 0.12f + 0.35f * progress;

        cairo_save(cr);
        draw_rounded_rect(cr, x - 6.0, y - 3.0, icon_size + 12.0, icon_size + 6.0, 10.0);
        set_source_color(cr, lighten(server->ui_state.panel_base, hovered ? 0.0f : 0.18f), opacity * halo_opacity);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        draw_rounded_rect(cr, x, y, icon_size, icon_size, 8.0);
        const float accent_mix = hovered ? 0.0f : 0.55f;
        set_source_color(cr, lighten(server->ui_state.accent_color, accent_mix), opacity * (0.6f + 0.4f * progress));
        cairo_fill(cr);
        cairo_restore(cr);

        if (server->pango_layout) {
            apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
            draw_text(cr, server->pango_layout, app.icon_label, x + 6.0, y + 6.0,
                      SwissDesign::WHITE, opacity);
        }

        x += icon_size + spacing;
    }
}

void draw_tray_icons(cairo_t *cr, const ArolloaServer *server, int width, float opacity) {
    double x = static_cast<double>(width) - 20.0;
    const double icon_size = 24.0;

    for (int index = static_cast<int>(server->ui_state.tray_icons.size()) - 1; index >= 0; --index) {
        const auto &indicator = server->ui_state.tray_icons[static_cast<std::size_t>(index)];
        const bool hovered = index == server->ui_state.hovered_tray_index;
        const float progress = hovered ? server->ui_state.tray_hover_progress : 0.0f;

        x -= icon_size;
        cairo_save(cr);
        draw_rounded_rect(cr, x - 6.0, SwissDesign::PANEL_HEIGHT / 2.0 - icon_size / 2.0 - 4.0,
                          icon_size + 12.0, icon_size + 8.0, 9.0);
        set_source_color(cr, lighten(server->ui_state.panel_base, hovered ? 0.05f : 0.15f), opacity * (0.2f + 0.4f * progress));
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_arc(cr, x + icon_size / 2.0, SwissDesign::PANEL_HEIGHT / 2.0, icon_size / 2.4, 0, 2 * kPi);
        set_source_color(cr, indicator.color, opacity * (0.65f + 0.35f * progress));
        cairo_fill(cr);
        cairo_restore(cr);

        if (server->pango_layout) {
            apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
            draw_text(cr, server->pango_layout, indicator.label, x - 4.0,
                      SwissDesign::PANEL_HEIGHT / 2.0 - 7.0, server->ui_state.panel_text,
                      opacity, PANGO_ALIGN_LEFT);
        }

        x -= 20.0;
    }
}

void draw_panel_branding(cairo_t *cr, const ArolloaServer *server, float opacity) {
    if (!server->pango_layout) {
        return;
    }
    apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 15);
    draw_text(cr, server->pango_layout, "AROLLOA", 20.0, SwissDesign::PANEL_HEIGHT / 2.0 - 9.0,
              server->ui_state.panel_text, opacity);

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
    draw_text(cr, server->pango_layout, "SWISS MENU", FOREST_PANEL_MENU_WIDTH - 20.0,
              SwissDesign::PANEL_HEIGHT / 2.0 - 6.0, lighten(server->ui_state.panel_text, 0.4f),
              opacity, PANGO_ALIGN_RIGHT);
}

void draw_panel_debug(cairo_t *cr, const ArolloaServer *server, int width, float opacity) {
    if (!server->pango_layout) {
        return;
    }
    apply_font(server->pango_layout, SwissDesign::MONO_FONT, 9);
    draw_text(cr, server->pango_layout, format_debug_info(server), width * 0.36,
              SwissDesign::PANEL_HEIGHT / 2.0 - 6.0, lighten(server->ui_state.panel_text, 0.55f), opacity * 0.8f);
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
    set_source_color(cr, SwissDesign::BLACK, 0.35f * opacity);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    const double panel_width = std::min<double>(FOREST_LAUNCHER_WIDTH, width - 120.0);
    const double panel_height = std::min<double>(height * 0.62,
        std::max<double>(SwissDesign::PANEL_HEIGHT * 5.0,
            server->ui_state.launcher_entries.size() * FOREST_LAUNCHER_ENTRY_HEIGHT + 160.0));
    const double start_x = (width - panel_width) / 2.0;
    const double start_y = (height - panel_height) / 2.0;

    draw_rounded_rect(cr, start_x, start_y, panel_width, panel_height, 22.0);
    set_source_color(cr, lighten(server->ui_state.panel_base, 0.04f), 0.98f * opacity);
    cairo_fill(cr);

    cairo_save(cr);
    draw_rounded_rect(cr, start_x, start_y, panel_width, 64.0, 22.0);
    set_source_color(cr, server->ui_state.accent_color, 0.12f * opacity);
    cairo_fill(cr);
    cairo_restore(cr);

    apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 18);
    draw_text(cr, server->pango_layout, "Swiss Application Grid", start_x + 36.0, start_y + 24.0,
              server->ui_state.panel_text, opacity);

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 11);
    draw_text(cr, server->pango_layout, "Curated workspaces, tools, and services",
              start_x + 36.0, start_y + 48.0, lighten(server->ui_state.panel_text, 0.35f), opacity * 0.9f);

    double entry_y = start_y + 96.0;
    std::size_t index = 0;
    for (const auto &entry : server->ui_state.launcher_entries) {
        const bool highlighted = index == server->ui_state.highlighted_index;
        cairo_save(cr);
        draw_rounded_rect(cr, start_x + 32.0, entry_y, panel_width - 64.0, FOREST_LAUNCHER_ENTRY_HEIGHT - 10.0, 14.0);
        if (highlighted) {
            set_source_color(cr, server->ui_state.accent_color, 0.55f * opacity);
        } else {
            set_source_color(cr, lighten(server->ui_state.panel_base, 0.1f), 0.5f * opacity);
        }
        cairo_fill(cr);
        cairo_restore(cr);

        apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 15);
        draw_text(cr, server->pango_layout, entry.name, start_x + 56.0, entry_y + 14.0,
                  highlighted ? SwissDesign::WHITE : server->ui_state.panel_text, opacity);

        apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
        draw_text(cr, server->pango_layout, entry.description, start_x + 56.0, entry_y + 36.0,
                  lighten(server->ui_state.panel_text, highlighted ? 0.6f : 0.35f), opacity * 0.9f);

        apply_font(server->pango_layout, SwissDesign::MONO_FONT, 9);
        draw_text(cr, server->pango_layout, entry.category, start_x + panel_width - 92.0,
                  entry_y + 16.0, lighten(server->ui_state.panel_text, 0.5f), opacity, PANGO_ALIGN_RIGHT);

        entry_y += FOREST_LAUNCHER_ENTRY_HEIGHT;
        ++index;
    }

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 9);
    draw_text(cr, server->pango_layout, "Hint: Super + Space toggles the application grid",
              start_x + 36.0, start_y + panel_height - 48.0, lighten(server->ui_state.panel_text, 0.45f), opacity * 0.85f);

    cairo_restore(cr);
}

void render_notifications(cairo_t *cr, ArolloaServer *server, int width, float opacity) {
    if (!server->pango_layout || !server->ui_state.notifications_enabled) {
        return;
    }

    double y = SwissDesign::PANEL_HEIGHT + 24.0;
    const double card_width = 320.0;
    const double spacing = 16.0;
    int count = 0;

    for (auto it = server->ui_state.notifications.rbegin(); it != server->ui_state.notifications.rend() && count < 4; ++it, ++count) {
        const float card_opacity = opacity * it->opacity;
        if (card_opacity <= 0.01f) {
            continue;
        }

        const double card_height = 80.0;
        const double x = width - card_width - 36.0;

        cairo_save(cr);
        draw_rounded_rect(cr, x, y, card_width, card_height, 14.0);
        set_source_color(cr, lighten(server->ui_state.panel_base, 0.12f), card_opacity);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        draw_rounded_rect(cr, x, y, 6.0, card_height, 14.0);
        set_source_color(cr, it->accent, card_opacity * 0.9f);
        cairo_fill(cr);
        cairo_restore(cr);

        apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 13);
        draw_text(cr, server->pango_layout, it->title, x + 20.0, y + 16.0,
                  server->ui_state.panel_text, card_opacity);

        apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
        draw_text(cr, server->pango_layout, it->body, x + 20.0, y + 40.0,
                  lighten(server->ui_state.panel_text, 0.4f), card_opacity * 0.9f);

        y += card_height + spacing;
    }
}

void render_volume_overlay(cairo_t *cr, ArolloaServer *server, int width, int height, float opacity) {
    const float visibility = server->ui_state.volume_feedback.visibility;
    if (visibility <= 0.01f || !server->pango_layout || !server->ui_state.notifications_enabled) {
        return;
    }

    const double overlay_width = 260.0;
    const double overlay_height = 180.0;
    const double x = (width - overlay_width) / 2.0;
    const double y = height * 0.68 - overlay_height / 2.0;

    cairo_save(cr);
    draw_rounded_rect(cr, x, y, overlay_width, overlay_height, 24.0);
    set_source_color(cr, lighten(server->ui_state.panel_base, 0.08f), opacity * visibility);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_save(cr);
    cairo_arc(cr, x + overlay_width / 2.0, y + 46.0, 26.0, 0, 2 * kPi);
    set_source_color(cr, server->ui_state.accent_color, opacity * visibility * 0.85f);
    cairo_fill(cr);
    cairo_restore(cr);

    const double track_x = x + 48.0;
    const double track_y = y + 108.0;
    const double track_width = overlay_width - 96.0;
    const double track_height = 10.0;
    const double fill_width = track_width * (server->ui_state.volume_feedback.level / 100.0);

    cairo_save(cr);
    draw_rounded_rect(cr, track_x, track_y, track_width, track_height, 5.0);
    set_source_color(cr, lighten(server->ui_state.panel_base, 0.25f), opacity * visibility * 0.5f);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_save(cr);
    draw_rounded_rect(cr, track_x, track_y, fill_width, track_height, 5.0);
    set_source_color(cr, server->ui_state.accent_color, opacity * visibility * 0.85f);
    cairo_fill(cr);
    cairo_restore(cr);

    apply_font(server->pango_layout, SwissDesign::PRIMARY_FONT, 28);
    draw_text_center(cr, server->pango_layout, std::to_string(server->ui_state.volume_feedback.level) + "%",
                     x + overlay_width / 2.0, y + 126.0,
                     server->ui_state.panel_text, opacity * visibility);

    apply_font(server->pango_layout, SwissDesign::SECONDARY_FONT, 10);
    draw_text_center(cr, server->pango_layout, "Volume", x + overlay_width / 2.0, y + 154.0,
                     lighten(server->ui_state.panel_text, 0.4f), opacity * visibility);
}

} // namespace

void render_swiss_panel(cairo_t *cairo, int width, int height, float opacity, const ArolloaServer *server) {
    (void)height;
    cairo_save(cairo);
    cairo_rectangle(cairo, 0, 0, width, SwissDesign::PANEL_HEIGHT);
    set_source_color(cairo, server->ui_state.panel_base, opacity);
    cairo_fill(cairo);
    cairo_restore(cairo);

    if (server->ui_state.menu_hover_progress > 0.01f) {
        cairo_save(cairo);
        cairo_rectangle(cairo, 0, 0, FOREST_PANEL_MENU_WIDTH, SwissDesign::PANEL_HEIGHT);
        const float intensity = 0.12f + server->ui_state.menu_hover_progress * 0.32f;
        set_source_color(cairo, server->ui_state.accent_color, opacity * intensity);
        cairo_fill(cairo);
        cairo_restore(cairo);
    }

    cairo_save(cairo);
    cairo_rectangle(cairo, 0, SwissDesign::PANEL_HEIGHT - 1.0, width, 1.0);
    set_source_color(cairo, SwissDesign::BLACK, opacity * 0.08f);
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

    if (!view->xdg_surface || !view->xdg_surface->surface) {
        return;
    }

    const int width = view->xdg_surface->surface->current.width;
    const int height = view->xdg_surface->surface->current.height;
    if (width <= 0 || height <= 0) {
        return;
    }

    const float opacity = view->opacity * global_opacity;
    if (opacity <= 0.0f) {
        return;
    }

    const double header_height = 34.0;
    const double shadow_radius = SwissDesign::CORNER_RADIUS + 6.0;
    const double frame_x = view->x - 8.0;
    const double frame_y = view->y - header_height - 10.0;
    const double frame_width = width + 16.0;
    const double frame_height = header_height + height + 18.0;

    cairo_save(cairo);
    draw_rounded_rect(cairo, frame_x, frame_y, frame_width, frame_height, shadow_radius);
    set_source_color(cairo, SwissDesign::BLACK, 0.14f * opacity);
    cairo_fill(cairo);
    cairo_restore(cairo);

    cairo_save(cairo);
    const double chrome_x = view->x - 2.0;
    const double chrome_y = view->y - header_height;
    const double chrome_width = width + 4.0;
    draw_rounded_rect(cairo, chrome_x, chrome_y, chrome_width, header_height + 4.0, SwissDesign::CORNER_RADIUS + 2.0);
    set_source_color(cairo, lighten(view->server->ui_state.panel_base, 0.08f), opacity * 0.96f);
    cairo_fill(cairo);
    cairo_restore(cairo);

    cairo_save(cairo);
    cairo_rectangle(cairo, chrome_x, chrome_y, chrome_width, 3.0);
    set_source_color(cairo, view->server->ui_state.accent_color, opacity * 0.9f);
    cairo_fill(cairo);
    cairo_restore(cairo);

    const char *title = "";
    if (view->xdg_surface->toplevel && view->xdg_surface->toplevel->title) {
        title = view->xdg_surface->toplevel->title;
    }

    if (view->server->pango_layout) {
        apply_font(view->server->pango_layout, SwissDesign::PRIMARY_FONT, 12);
        draw_text(cairo, view->server->pango_layout, title ? title : "Untitled",
                  chrome_x + 16.0, chrome_y + 10.0, view->server->ui_state.panel_text, opacity);
    }

    cairo_save(cairo);
    const double controls_center_y = chrome_y + header_height / 2.0 + 2.0;
    const double control_spacing = 18.0;
    double control_x = chrome_x + chrome_width - 28.0;
    set_source_color(cairo, view->server->ui_state.accent_color, opacity * 0.85f);
    cairo_arc(cairo, control_x, controls_center_y, 6.0, 0, 2 * kPi);
    cairo_fill(cairo);
    control_x -= control_spacing;
    set_source_color(cairo, lighten(view->server->ui_state.panel_text, 0.4f), opacity * 0.7f);
    cairo_arc(cairo, control_x, controls_center_y, 6.0, 0, 2 * kPi);
    cairo_fill(cairo);
    control_x -= control_spacing;
    set_source_color(cairo, lighten(view->server->ui_state.panel_text, 0.2f), opacity * 0.5f);
    cairo_arc(cairo, control_x, controls_center_y, 6.0, 0, 2 * kPi);
    cairo_fill(cairo);
    cairo_restore(cairo);
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

    ArolloaView *decorated = nullptr;
    wl_list_for_each(decorated, &server->views, link) {
        render_swiss_window(server->cairo_ctx, decorated, opacity);
    }

    render_notifications(server->cairo_ctx, server, width, opacity);
    render_volume_overlay(server->cairo_ctx, server, width, height, opacity);

    cairo_surface_flush(server->ui_surface);
}

void initialize_forest_ui(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->ui_state.accent_color = color_from_hex(get_config_string("colors.accent", "#d4001a"), SwissDesign::SWISS_RED);
    server->ui_state.panel_base = color_from_hex(get_config_string("colors.panel", "#ffffff"), SwissDesign::WHITE);
    server->ui_state.panel_text = color_from_hex(get_config_string("colors.panel_text", "#1a1a1a"), SwissDesign::BLACK);
    server->ui_state.notifications_enabled = get_config_bool("notifications.enabled", true);

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
    server->ui_state.notifications.clear();
    server->ui_state.volume_feedback.visibility = 0.0f;
    server->ui_state.volume_feedback.target_visibility = 0.0f;
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
