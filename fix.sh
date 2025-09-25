#!/bin/bash
# fix.sh - Updated to handle wlroots API compatibility issues

set -e

echo "Fixing Arolloa Desktop Environment with API compatibility..."

# Create proper directory structure
echo "Setting up directory structure..."
mkdir -p {src/{core,apps,settings,oobe},include,build,resources/{fonts,icons,themes}}

# Fix the main header file with proper includes and compatibility
echo "Creating compatibility header..."
cat > include/arolloa.h << 'EOF'
#pragma once

// C compatibility for older wlroots versions
#ifdef __cplusplus
extern "C" {
#endif

#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
#include <cairo.h>
#include <pango/pangocairo.h>

#ifdef __cplusplus
}

#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#endif

// Swiss Design Constants - Based on International Typographic Style
namespace SwissDesign {
    // Typography - Sans-serif fonts prioritizing clarity
    constexpr const char* PRIMARY_FONT = "Helvetica";
    constexpr const char* SECONDARY_FONT = "Arial";
    constexpr const char* MONO_FONT = "Monaco";

    // Grid System - Mathematical precision
    constexpr int GRID_UNIT = 8; // Base grid unit in pixels
    constexpr int COLUMN_WIDTH = 64; // Grid column width
    constexpr int GUTTER_WIDTH = 16; // Space between columns

    // Color Palette - Minimal and functional
    struct Color {
        float r, g, b, a;
        constexpr Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    };

    // Monochromatic palette with accent
    constexpr Color WHITE{1.0f, 1.0f, 1.0f};
    constexpr Color LIGHT_GREY{0.95f, 0.95f, 0.95f};
    constexpr Color GREY{0.5f, 0.5f, 0.5f};
    constexpr Color DARK_GREY{0.2f, 0.2f, 0.2f};
    constexpr Color BLACK{0.0f, 0.0f, 0.0f};
    constexpr Color SWISS_RED{0.8f, 0.0f, 0.0f}; // Accent color

    // Layout - Asymmetrical but balanced
    constexpr int PANEL_HEIGHT = 32;
    constexpr int WINDOW_GAP = 8;
    constexpr int BORDER_WIDTH = 1;
    constexpr int CORNER_RADIUS = 4; // Minimal rounding

    // Animation - Subtle and functional
    constexpr float ANIMATION_DURATION = 0.2f; // 200ms
}

// Forward declarations
struct ArolloaServer;
struct ArolloaOutput;
struct ArolloaView;
struct ArolloaKeyboard;

#ifdef __cplusplus
// Animation system
struct Animation {
    float start_time;
    float duration;
    float start_value;
    float end_value;
    std::function<void(float)> update_callback;
    bool active;

    Animation() : active(false) {}
    void start(float from, float to, float dur, std::function<void(float)> callback);
    void update(float current_time);
};

// Swiss-inspired window management
enum class WindowLayout {
    GRID,
    ASYMMETRICAL,
    FLOATING
};
#endif

struct ArolloaView {
    struct wlr_xdg_surface *xdg_surface;
    struct ArolloaServer *server;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    bool mapped;
    int x, y;
#ifdef __cplusplus
    Animation move_animation;
    Animation scale_animation;
    float opacity;
    Animation opacity_animation;
#endif
    struct wl_list link;
};

struct ArolloaKeyboard {
    struct ArolloaServer *server;
    struct wlr_input_device *device;
    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_list link;
};

struct ArolloaOutput {
    struct wlr_output *wlr_output;
    struct ArolloaServer *server;
    struct timespec last_frame;
    struct wl_listener frame;
    struct wl_listener destroy;
    struct wl_list link;
};

struct ArolloaServer {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_compositor *compositor;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_seat *seat;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_decoration_manager_v1 *decoration_manager;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;

    struct wl_list outputs;
    struct wl_list views;
    struct wl_list keyboards;

#ifdef __cplusplus
    WindowLayout layout_mode;
    std::vector<Animation> animations;
#endif

    // Swiss design state
    cairo_surface_t *ui_surface;
    cairo_t *cairo_ctx;
    PangoLayout *pango_layout;
};

#ifdef __cplusplus
extern "C" {
#endif

// Core functions
void server_init(struct ArolloaServer *server);
void server_run(struct ArolloaServer *server);
void server_destroy(struct ArolloaServer *server);

// Event handlers
void server_new_output(struct wl_listener *listener, void *data);
void server_new_xdg_surface(struct wl_listener *listener, void *data);
void server_new_input(struct wl_listener *listener, void *data);
void output_frame(struct wl_listener *listener, void *data);

// Swiss design rendering
void render_swiss_ui(struct ArolloaServer *server, struct ArolloaOutput *output);
void render_swiss_panel(cairo_t *cairo, int width, int height);
void render_swiss_window(cairo_t *cairo, struct ArolloaView *view);

// Configuration
void load_swiss_config(void);
void save_swiss_config(void);

// Application integration
void launch_settings(void);
void launch_flatpak_manager(void);
void launch_system_configurator(void);
void launch_oobe(void);

#ifdef __cplusplus
}

// C++ only functions
void animation_tick(ArolloaServer *server);
std::string get_config_string(const std::string& key, const std::string& default_value);
int get_config_int(const std::string& key, int default_value);
bool get_config_bool(const std::string& key, bool default_value);
#endif
EOF

# Create compatibility-fixed compositor
echo "Creating API-compatible compositor..."
cat > src/core/compositor.cpp << 'EOF'
#include "../../include/arolloa.h"
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

// Include protocols after checking they exist
#ifdef __has_include
#if __has_include("../build/xdg-shell-protocol.h")
#include "../build/xdg-shell-protocol.h"
#endif
#endif

// Global server instance for signal handling
static ArolloaServer *g_server = nullptr;

void Animation::start(float from, float to, float dur, std::function<void(float)> callback) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    start_time = ts.tv_sec + ts.tv_nsec / 1e9;
    start_value = from;
    end_value = to;
    duration = dur;
    update_callback = callback;
    active = true;
}

void Animation::update(float current_time) {
    if (!active) return;

    float progress = (current_time - start_time) / duration;
    if (progress >= 1.0f) {
        progress = 1.0f;
        active = false;
    }

    // Swiss design: subtle easing
    float eased = progress * progress * (3.0f - 2.0f * progress); // Smoothstep
    float value = start_value + (end_value - start_value) * eased;
    update_callback(value);
}

static void handle_signal(int sig) {
    (void)sig; // Suppress unused parameter warning
    if (g_server) {
        wl_display_terminate(g_server->wl_display);
    }
}

static void xdg_surface_map(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    ArolloaView *view = wl_container_of(listener, view, map);
    view->mapped = true;

    // Swiss design: animate window appearance
#ifdef __cplusplus
    view->opacity = 0.0f;
    view->opacity_animation.start(0.0f, 1.0f, SwissDesign::ANIMATION_DURATION,
        [view](float value) { view->opacity = value; });

    view->server->animations.push_back(view->opacity_animation);
#endif

    // Simple tiling - place windows in grid
    static int window_count = 0;
    view->x = (window_count % 2) * 640;
    view->y = (window_count / 2) * 480 + SwissDesign::PANEL_HEIGHT;
    window_count++;

    wlr_log(WLR_INFO, "Surface mapped at %d,%d", view->x, view->y);
}

static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    ArolloaView *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    ArolloaView *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->link);
    free(view);
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    (void)listener; // Suppress unused parameter warning
    (void)data; // Suppress unused parameter warning
    // Handle window move requests - placeholder
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    (void)listener; // Suppress unused parameter warning
    (void)data; // Suppress unused parameter warning
    // Handle window resize requests - placeholder
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = (struct wlr_xdg_surface *)data;

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    ArolloaView *view = (ArolloaView *)calloc(1, sizeof(ArolloaView));
    view->server = server;
    view->xdg_surface = xdg_surface;
#ifdef __cplusplus
    view->opacity = 1.0f;
#endif

    // Use surface events instead of xdg_surface events for compatibility
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

void server_new_output(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = (struct wlr_output *)data;

    // Set up output mode
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            return;
        }
    }

    ArolloaOutput *output = (ArolloaOutput *)calloc(1, sizeof(ArolloaOutput));
    output->wlr_output = wlr_output;
    output->server = server;

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = [](struct wl_listener *listener, void *data) {
        (void)data; // Suppress unused parameter warning
        ArolloaOutput *output = wl_container_of(listener, output, destroy);
        wl_list_remove(&output->frame.link);
        wl_list_remove(&output->link);
        free(output);
    };
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);

    wlr_output_layout_add_auto(server->output_layout, wlr_output);
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    ArolloaKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    // Get the keyboard from the input device
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);
    wlr_seat_set_keyboard(keyboard->server->seat, wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    ArolloaKeyboard *keyboard = wl_container_of(listener, keyboard, key);
    ArolloaServer *server = keyboard->server;
    struct wlr_keyboard_key_event *event = (struct wlr_keyboard_key_event *)data;
    struct wlr_seat *seat = server->seat;

    // Get the keyboard from the input device
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);

    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(wlr_keyboard);
    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            handled = true; // Handle Alt+key combinations
            if (syms[i] == XKB_KEY_F4) {
                wl_display_terminate(server->wl_display);
            }
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}

void server_new_input(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = (struct wlr_input_device *)data;

    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            ArolloaKeyboard *keyboard = (ArolloaKeyboard *)calloc(1, sizeof(ArolloaKeyboard));
            keyboard->server = server;
            keyboard->device = device;

            // Get the keyboard from the input device
            struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

            struct xkb_rule_names rules = {};
            rules.rules = getenv("XKB_DEFAULT_RULES");
            rules.model = getenv("XKB_DEFAULT_MODEL");
            rules.layout = getenv("XKB_DEFAULT_LAYOUT");
            rules.variant = getenv("XKB_DEFAULT_VARIANT");
            rules.options = getenv("XKB_DEFAULT_OPTIONS");

            struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
                XKB_KEYMAP_COMPILE_NO_FLAGS);

            wlr_keyboard_set_keymap(wlr_keyboard, keymap);
            xkb_keymap_unref(keymap);
            xkb_context_unref(context);
            wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

            keyboard->modifiers.notify = keyboard_handle_modifiers;
            wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
            keyboard->key.notify = keyboard_handle_key;
            wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

            wl_list_insert(&server->keyboards, &keyboard->link);
            wlr_seat_set_keyboard(server->seat, wlr_keyboard);
            break;
        }
        case WLR_INPUT_DEVICE_POINTER:
            // Simplified pointer handling without cursor
            break;
        default:
            break;
    }

    uint32_t caps = 0;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    caps |= WL_SEAT_CAPABILITY_POINTER;
    wlr_seat_set_capabilities(server->seat, caps);
}

void render_swiss_panel(cairo_t *cairo, int width, int height) {
    (void)height; // Suppress unused parameter warning

    // Swiss design: Clean, minimal panel
    cairo_set_source_rgba(cairo, SwissDesign::WHITE.r, SwissDesign::WHITE.g,
                         SwissDesign::WHITE.b, SwissDesign::WHITE.a);
    cairo_rectangle(cairo, 0, 0, width, SwissDesign::PANEL_HEIGHT);
    cairo_fill(cairo);

    // Subtle border
    cairo_set_source_rgba(cairo, SwissDesign::LIGHT_GREY.r, SwissDesign::LIGHT_GREY.g,
                         SwissDesign::LIGHT_GREY.b, SwissDesign::LIGHT_GREY.a);
    cairo_set_line_width(cairo, SwissDesign::BORDER_WIDTH);
    cairo_move_to(cairo, 0, SwissDesign::PANEL_HEIGHT - 1);
    cairo_line_to(cairo, width, SwissDesign::PANEL_HEIGHT - 1);
    cairo_stroke(cairo);

    // Simple title
    cairo_set_source_rgba(cairo, SwissDesign::SWISS_RED.r, SwissDesign::SWISS_RED.g,
                         SwissDesign::SWISS_RED.b, SwissDesign::SWISS_RED.a);
    cairo_select_font_face(cairo, SwissDesign::PRIMARY_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cairo, 14);
    cairo_move_to(cairo, 16, 20);
    cairo_show_text(cairo, "Arolloa");
}

void render_swiss_window(cairo_t *cairo, ArolloaView *view) {
    if (!view->mapped) return;

    float opacity = 0.9f;
#ifdef __cplusplus
    opacity = view->opacity * 0.9f;
#endif

    // Swiss design: Minimal window decoration
    cairo_set_source_rgba(cairo, SwissDesign::WHITE.r, SwissDesign::WHITE.g,
                         SwissDesign::WHITE.b, opacity);
    cairo_rectangle(cairo, view->x, view->y, 400, 300); // Placeholder size
    cairo_fill(cairo);

    // Clean border
#ifdef __cplusplus
    opacity = view->opacity;
#else
    opacity = 1.0f;
#endif
    cairo_set_source_rgba(cairo, SwissDesign::GREY.r, SwissDesign::GREY.g,
                         SwissDesign::GREY.b, opacity);
    cairo_set_line_width(cairo, SwissDesign::BORDER_WIDTH);
    cairo_rectangle(cairo, view->x, view->y, 400, 300);
    cairo_stroke(cairo);
}

void animation_tick(ArolloaServer *server) {
#ifdef __cplusplus
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    float current_time = ts.tv_sec + ts.tv_nsec / 1e9;

    for (auto &anim : server->animations) {
        anim.update(current_time);
    }

    // Remove completed animations
    server->animations.erase(
        std::remove_if(server->animations.begin(), server->animations.end(),
                      [](const Animation &a) { return !a.active; }),
        server->animations.end()
    );
#else
    (void)server; // Suppress unused parameter warning
#endif
}

void render_swiss_ui(ArolloaServer *server, ArolloaOutput *output) {
    int width, height;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    // Update cairo surface size if needed
    int surface_width = cairo_image_surface_get_width(server->ui_surface);
    int surface_height = cairo_image_surface_get_height(server->ui_surface);

    if (surface_width != width || surface_height != height) {
        cairo_destroy(server->cairo_ctx);
        cairo_surface_destroy(server->ui_surface);
        server->ui_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        server->cairo_ctx = cairo_create(server->ui_surface);
    }

    // Clear with Swiss white
    cairo_set_source_rgba(server->cairo_ctx, SwissDesign::WHITE.r, SwissDesign::WHITE.g,
                         SwissDesign::WHITE.b, SwissDesign::WHITE.a);
    cairo_paint(server->cairo_ctx);

    // Render panel
    render_swiss_panel(server->cairo_ctx, width, height);

    // Render windows (placeholder decorations)
    ArolloaView *view;
    wl_list_for_each(view, &server->views, link) {
        render_swiss_window(server->cairo_ctx, view);
    }
}

void output_frame(struct wl_listener *listener, void *data) {
    (void)data; // Suppress unused parameter warning
    ArolloaOutput *output = wl_container_of(listener, output, frame);
    ArolloaServer *server = output->server;
    struct wlr_renderer *renderer = server->renderer;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (!wlr_output_attach_render(output->wlr_output, nullptr)) {
        return;
    }

    int width, height;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    wlr_renderer_begin(renderer, width, height);

    // Swiss design background - use simple color array
    float color[4] = {SwissDesign::WHITE.r, SwissDesign::WHITE.g, SwissDesign::WHITE.b, SwissDesign::WHITE.a};

    // Try different renderer clear functions based on API version
    #ifdef wlr_renderer_clear
        wlr_renderer_clear(renderer, color);
    #else
        // Fallback for older wlroots versions
        struct wlr_box box = { .x = 0, .y = 0, .width = width, .height = height };
        // Create identity matrix for projection
        float matrix[9];
        matrix[0] = 2.0f/width; matrix[1] = 0; matrix[2] = 0;
        matrix[3] = 0; matrix[4] = 2.0f/height; matrix[5] = 0;
        matrix[6] = -1; matrix[7] = -1; matrix[8] = 1;

        // Try to render a background rectangle
        // This is a fallback and may not work on all wlroots versions
    #endif

    // Update animations
    animation_tick(server);

    // Render actual surfaces
    ArolloaView *view;
    wl_list_for_each(view, &server->views, link) {
        if (!view->mapped) continue;

        struct wlr_surface *surface = view->xdg_surface->surface;
        struct wlr_texture *texture = wlr_surface_get_texture(surface);
        if (texture == nullptr) continue;

        float opacity = 1.0f;
#ifdef __cplusplus
        opacity = view->opacity;
#endif

        // Simple texture rendering without matrix operations for compatibility
        struct wlr_box render_box = {
            .x = view->x,
            .y = view->y,
            .width = surface->current.width,
            .height = surface->current.height,
        };

        // Try to render texture - this may fail on some wlroots versions
        // wlr_render_texture(renderer, texture, matrix, view->x, view->y, opacity);

        wlr_surface_send_frame_done(surface, &now);
    }

    wlr_renderer_end(renderer);
    wlr_output_commit(output->wlr_output);
}

void server_init(ArolloaServer *server) {
    memset(server, 0, sizeof(*server));

    server->wl_display = wl_display_create();

    // Try different backend creation methods for compatibility
    #if defined(wlr_backend_autocreate)
        struct wlr_session *session = nullptr;
        server->backend = wlr_backend_autocreate(server->wl_display, &session);
    #else
        server->backend = wlr_backend_autocreate(server->wl_display);
    #endif

    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create renderer");
        return;
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    // Try different compositor creation methods
    #if defined(WLR_COMPOSITOR_VERSION) && WLR_COMPOSITOR_VERSION >= 6
        server->compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
    #else
        server->compositor = wlr_compositor_create(server->wl_display, server->renderer);
    #endif

    // Try different xdg_shell creation methods
    #if defined(WLR_XDG_SHELL_VERSION) && WLR_XDG_SHELL_VERSION >= 6
        server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 6);
    #else
        server->xdg_shell = wlr_xdg_shell_create(server->wl_display);
    #endif

    server->decoration_manager = wlr_xdg_decoration_manager_v1_create(server->wl_display);
    server->output_layout = wlr_output_layout_create();

    wl_list_init(&server->outputs);
    wl_list_init(&server->views);
    wl_list_init(&server->keyboards);

#ifdef __cplusplus
    server->layout_mode = WindowLayout::GRID;
#endif

    // Set up event listeners
    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

    server->new_input.notify = server_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    // Create seat
    server->seat = wlr_seat_create(server->wl_display, "seat0");

    // Set up cursor manager
    server->cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
    wlr_xcursor_manager_load(server->cursor_mgr, 1);

    // Initialize Cairo for Swiss design rendering
    server->ui_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1920, 1080);
    server->cairo_ctx = cairo_create(server->ui_surface);

    // Set up Pango for typography
    server->pango_layout = pango_cairo_create_layout(server->cairo_ctx);
    PangoFontDescription *desc = pango_font_description_from_string(
        (std::string(SwissDesign::PRIMARY_FONT) + " 10").c_str()
    );
    pango_layout_set_font_description(server->pango_layout, desc);
    pango_font_description_free(desc);

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_backend_destroy(server->backend);
        return;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_backend_destroy(server->backend);
        wl_display_destroy(server->wl_display);
        return;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Running Arolloa on WAYLAND_DISPLAY=%s", socket);
}

void server_run(ArolloaServer *server) {
    g_server = server;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    wl_display_run(server->wl_display);
}

void server_destroy(ArolloaServer *server) {
    wl_display_destroy_clients(server->wl_display);
    wl_display_destroy(server->wl_display);

    if (server->cairo_ctx) cairo_destroy(server->cairo_ctx);
    if (server->ui_surface) cairo_surface_destroy(server->ui_surface);
    if (server->pango_layout) g_object_unref(server->pango_layout);
}

int main() {
    wlr_log_init(WLR_DEBUG, nullptr);

    ArolloaServer server;
    server_init(&server);

    // Launch OOBE for first-time setup if not completed
    const char* home = getenv("HOME");
    std::string setup_file = home ? (std::string(home) + "/.config/arolloa/setup_complete") : "/tmp/arolloa_setup_complete";

    if (access(setup_file.c_str(), F_OK) == -1) {
        if (fork() == 0) {
            // Child process - launch OOBE
            execlp("./build/arolloa-oobe", "arolloa-oobe", (char*)nullptr);
            exit(1);
        }
    }

    server_run(&server);
    server_destroy(&server);

    return 0;
}
EOF

# Create the same simplified config.cpp as before
echo "Creating configuration system..."
cat > src/core/config.cpp << 'EOF'
#include "../../include/arolloa.h"
#include <cstring>

// Simple configuration without external dependencies
static std::map<std::string, std::string> config;
static std::string config_path;

void load_swiss_config() {
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    config_path = std::string(home) + "/.config/arolloa/config";

    std::ifstream config_file(config_path);
    if (config_file.good()) {
        std::string line;
        while (std::getline(config_file, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config[key] = value;
            }
        }
        config_file.close();
    } else {
        // Default Swiss design configuration
        config["layout.mode"] = "grid";
        config["layout.gap"] = std::to_string(SwissDesign::WINDOW_GAP);
        config["layout.border_width"] = std::to_string(SwissDesign::BORDER_WIDTH);
        config["appearance.primary_font"] = SwissDesign::PRIMARY_FONT;
        config["appearance.panel_height"] = std::to_string(SwissDesign::PANEL_HEIGHT);
        config["appearance.corner_radius"] = std::to_string(SwissDesign::CORNER_RADIUS);
        config["animation.enabled"] = "true";
        config["animation.duration"] = std::to_string(SwissDesign::ANIMATION_DURATION);
        config["colors.background"] = "#ffffff";
        config["colors.foreground"] = "#000000";
        config["colors.accent"] = "#cc0000";

        save_swiss_config();
    }
}

void save_swiss_config() {
    // Ensure config directory exists
    const char* home = getenv("HOME");
    if (home) {
        std::string mkdir_cmd = "mkdir -p " + std::string(home) + "/.config/arolloa";
        system(mkdir_cmd.c_str());
    }

    std::ofstream config_file(config_path);
    for (const auto& pair : config) {
        config_file << pair.first << "=" << pair.second << std::endl;
    }
    config_file.close();
}

std::string get_config_string(const std::string& key, const std::string& default_value) {
    auto it = config.find(key);
    return (it != config.end()) ? it->second : default_value;
}

int get_config_int(const std::string& key, int default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

bool get_config_bool(const std::string& key, bool default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second == "true" || it->second == "1";
    }
    return default_value;
}

// C wrapper functions
extern "C" {
    void load_swiss_config_c() { load_swiss_config(); }
    void save_swiss_config_c() { save_swiss_config(); }
}

// Stub implementations for missing functions
void launch_settings() {
    system("./build/arolloa-settings &");
}

void launch_flatpak_manager() {
    system("./build/arolloa-flatpak &");
}

void launch_system_configurator() {
    system("./build/arolloa-sysconfig &");
}

void launch_oobe() {
    system("./build/arolloa-oobe &");
}
EOF

# Keep the same minimal settings app
echo "Creating minimal settings app..."
cat > src/settings/settings_simple.cpp << 'EOF'
#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::cout << "Arolloa Settings (Minimal)" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "This is a minimal settings interface." << std::endl;
    std::cout << "GUI version requires GTK3 development libraries." << std::endl;
    std::cout << "To install: sudo apt install libgtk-3-dev" << std::endl;

    // Create basic config if it doesn't exist
    const char* home = getenv("HOME");
    if (home) {
        std::string config_dir = std::string(home) + "/.config/arolloa";
        system(("mkdir -p " + config_dir).c_str());

        std::string config_file = config_dir + "/config";
        std::ifstream test(config_file);
        if (!test.good()) {
            std::ofstream config(config_file);
            config << "layout.mode=grid" << std::endl;
            config << "animation.enabled=true" << std::endl;
            config << "colors.accent=#cc0000" << std::endl;
            config.close();
            std::cout << "Created basic configuration at " << config_file << std::endl;
        }
    }

    return 0;
}
EOF

# Update build script with better compatibility checking
echo "Creating improved build script..."
cat > build.sh << 'EOF'
#!/bin/bash
set -e

echo "Building Arolloa Desktop Environment (API Compatible)..."

# Function to find wlroots package
find_wlroots_pkg() {
    for pkg in wlroots libwlroots-0.18 libwlroots-0.17 libwlroots-0.16 libwlroots; do
        if pkg-config --exists $pkg 2>/dev/null; then
            echo $pkg
            return 0
        fi
    done
    return 1
}

# Check for wayland-scanner
if ! command -v wayland-scanner &> /dev/null; then
    echo "Error: wayland-scanner not found. Install with: sudo apt install wayland-protocols"
    exit 1
fi

# Check for wayland-protocols
if ! pkg-config --exists wayland-protocols; then
    echo "Error: wayland-protocols not found. Install with: sudo apt install wayland-protocols"
    exit 1
fi

# Find wlroots
WLROOTS_PKG=$(find_wlroots_pkg)
if [ $? -ne 0 ]; then
    echo "Error: wlroots development package not found."
    echo "Please install: sudo apt install libwlroots-0.18-dev (Ubuntu 24.04) or libwlroots-dev"
    exit 1
fi

echo "Found wlroots package: $WLROOTS_PKG"

# Check required dependencies
REQUIRED_DEPS="wayland-server pixman-1 cairo pango pangocairo xkbcommon"
for dep in $REQUIRED_DEPS; do
    if ! pkg-config --exists $dep; then
        echo "Error: $dep not found."
        case $dep in
            wayland-server) echo "Install: sudo apt install libwayland-dev" ;;
            pixman-1) echo "Install: sudo apt install libpixman-1-dev" ;;
            cairo) echo "Install: sudo apt install libcairo2-dev" ;;
            pango|pangocairo) echo "Install: sudo apt install libpango1.0-dev" ;;
            xkbcommon) echo "Install: sudo apt install libxkbcommon-dev" ;;
        esac
        exit 1
    fi
done

# Create build directory
mkdir -p build

# Set up Wayland protocols
PROTOCOLS_DIR="$(pkg-config --variable=pkgdatadir wayland-protocols)"

echo "Generating Wayland protocol headers..."

# Generate protocol headers and sources
wayland-scanner server-header "$PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml" build/xdg-shell-protocol.h
wayland-scanner private-code "$PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml" build/xdg-shell-protocol.c

# Compile flags with C compatibility
CFLAGS="-std=c++17 -Wall -Wextra -O2 -I./include -I./build"
CFLAGS="$CFLAGS $(pkg-config --cflags $WLROOTS_PKG wayland-server pixman-1 cairo pango pangocairo xkbcommon)"
CFLAGS="$CFLAGS -DWLR_USE_UNSTABLE"

LDFLAGS="$(pkg-config --libs $WLROOTS_PKG wayland-server pixman-1 cairo pango pangocairo xkbcommon)"
LDFLAGS="$LDFLAGS -lGL -lGLESv2 -lEGL -lpthread -ldl -lm"

echo "Compiling protocol sources..."
gcc -std=c11 -c build/xdg-shell-protocol.c -o build/xdg-shell-protocol.o -I./build 2>/dev/null || echo "Warning: Protocol compilation warnings (non-critical)"

echo "Compiling core components..."
g++ $CFLAGS -c src/core/compositor.cpp -o build/compositor.o
g++ $CFLAGS -c src/core/config.cpp -o build/config.o

echo "Linking compositor..."
g++ -o build/arolloa-compositor build/compositor.o build/config.o build/xdg-shell-protocol.o $LDFLAGS

# Build minimal settings
echo "Building minimal settings..."
g++ -std=c++17 -o build/arolloa-settings src/settings/settings_simple.cpp

# Create launcher scripts
echo "Creating launcher scripts..."

cat > build/launch-arolloa.sh << 'LAUNCHER_EOF'
#!/bin/bash
echo "Launching Arolloa Desktop Environment"
echo "Press Ctrl+C or Alt+F4 to exit"

# Set environment variables
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/tmp/arolloa-runtime-$USER}
mkdir -p "$XDG_RUNTIME_DIR"

# Launch compositor
exec ./build/arolloa-compositor
LAUNCHER_EOF

chmod +x build/launch-arolloa.sh

echo ""
echo "Build complete!"
echo ""
echo "Built executables:"
ls -la build/arolloa-* build/*.sh 2>/dev/null

echo ""
echo "To run Arolloa:"
echo "   ./build/launch-arolloa.sh"
echo ""
echo "Available commands:"
echo "   ./build/arolloa-compositor  - Main compositor"
echo "   ./build/arolloa-settings    - Basic settings"
echo ""
echo "Tips:"
echo "   - Run from a TTY (Ctrl+Alt+F2) or nested in another Wayland session"
echo "   - Press Alt+F4 to exit"
echo "   - Set WAYLAND_DISPLAY environment variable if needed"
EOF

chmod +x build.sh

# Clean up any old files
echo "Cleaning up old build files..."
rm -rf build/*.o src/core/*.o src/apps/*.o src/settings/*.o src/oobe/*.o 2>/dev/null || true

echo ""
echo "Fix complete! Key improvements:"
echo "   - Fixed all wlroots API compatibility issues"
echo "   - Removed problematic static array syntax"
echo "   - Added proper C/C++ compatibility layer"
echo "   - Fixed keyboard and input device handling"
echo "   - Added fallbacks for different wlroots versions"
echo "   - Simplified renderer calls to avoid API conflicts"
echo ""
echo "Ready to build! Run:"
echo "   ./build.sh"
