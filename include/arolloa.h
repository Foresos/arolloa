#pragma once

// C compatibility for older wlroots versions
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <wayland-server-core.h>

#pragma push_macro("static")
#undef static
#define static
#include <wlr/backend.h>
#if defined(__has_include)
#  if __has_include(<wlr/backend/session.h>)
#    include <wlr/backend/session.h>
#  endif
#endif
struct wlr_session;
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#if defined(__has_include)
#  if __has_include(<wlr/types/wlr_surface.h>)
#    include <wlr/types/wlr_surface.h>
#  endif
#else
#  include <wlr/types/wlr_surface.h>
#endif
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/log.h>
#include <wlr/util/box.h>
#pragma pop_macro("static")

#include <xkbcommon/xkbcommon.h>
#include <cairo.h>
#include <pango/pangocairo.h>

#ifdef __cplusplus
}

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#endif

// Swiss Design Constants - Based on International Typographic Style
namespace SwissDesign {
    // Typography - Sans-serif fonts prioritizing clarity
    constexpr const char* PRIMARY_FONT = "Helvetica";
    constexpr const char* SECONDARY_FONT = "Arial";
    constexpr const char* MONO_FONT = "Monaco";

#ifdef __cplusplus
    inline constexpr std::array<const char *, 6> PRIMARY_FONT_CANDIDATES = {
        "Inter",
        "Noto Sans",
        "DejaVu Sans",
        "Ubuntu",
        "Cantarell",
        PRIMARY_FONT,
    };

    inline constexpr std::array<const char *, 5> SECONDARY_FONT_CANDIDATES = {
        "Source Sans Pro",
        "IBM Plex Sans",
        "Roboto",
        SECONDARY_FONT,
        "Liberation Sans",
    };

    inline constexpr std::array<const char *, 4> MONO_FONT_CANDIDATES = {
        "JetBrains Mono",
        "Fira Mono",
        MONO_FONT,
        "monospace",
    };

    inline std::string build_font_stack(std::span<const char *const> fonts) {
        std::ostringstream oss;
        bool first = true;
        for (const char *font : fonts) {
            if (!font || font[0] == '\0') {
                continue;
            }
            if (!first) {
                oss << ", ";
            }
            oss << '"' << font << '"';
            first = false;
        }
        if (!first) {
            oss << ", ";
        }
        oss << "sans-serif";
        return oss.str();
    }

    inline std::string font_stack_css() {
        return build_font_stack(PRIMARY_FONT_CANDIDATES);
    }

    inline std::string mono_font_stack_css() {
        return build_font_stack(MONO_FONT_CANDIDATES);
    }
#endif

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
    struct wlr_xdg_toplevel *toplevel;
    struct ArolloaServer *server;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener request_minimize;
    struct wl_listener request_show_window_menu;
    struct wl_listener set_title;
    struct wl_listener set_app_id;
    struct wl_listener set_parent;
    bool mapped;
    int x, y;
    int width;
    int height;
    bool is_fullscreen;
    bool is_maximized;
    bool is_minimized;
#ifdef __cplusplus
    float opacity;
    std::string title;
    std::string app_id;
#else
    float opacity;
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
    struct wl_listener request_state;
    struct wl_listener destroy;
    struct wl_list link;
};

struct ArolloaServer {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_session *session;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
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
    struct wl_listener new_decoration;

    struct wl_list outputs;
    struct wl_list views;
    struct wl_list keyboards;

#ifdef __cplusplus
    WindowLayout layout_mode{WindowLayout::GRID};
    std::vector<std::unique_ptr<Animation>> animations;
    bool debug_mode{false};
    bool nested_backend_active{false};
    bool initialized{false};
    float startup_opacity{0.0f};
    ArolloaView *focused_view{nullptr};
    std::string primary_font;
    std::string secondary_font;
    std::string mono_font;
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
void render_swiss_panel(const struct ArolloaServer *server, cairo_t *cairo, int width, int height, float opacity);
void render_swiss_window(cairo_t *cairo, struct ArolloaView *view, float global_opacity);

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
void push_animation(ArolloaServer *server, std::unique_ptr<Animation> animation);
void schedule_startup_animation(ArolloaServer *server);
std::string get_config_string(const std::string& key, const std::string& default_value);
int get_config_int(const std::string& key, int default_value);
bool get_config_bool(const std::string& key, bool default_value);
void arrange_views(ArolloaServer *server);
void focus_view(ArolloaServer *server, ArolloaView *view);
void initialize_font_stack(ArolloaServer *server);
#endif
