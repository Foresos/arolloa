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
#include <wlr/types/wlr_cursor.h>
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
#include <chrono>
#include <cmath>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
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

    namespace Forest {
        constexpr Color CANOPY_DARK{0.047f, 0.121f, 0.086f};
        constexpr Color CANOPY_MID{0.121f, 0.227f, 0.164f};
        constexpr Color CANOPY_LIGHT{0.247f, 0.372f, 0.250f};
        constexpr Color MOSS_ACCENT{0.419f, 0.584f, 0.305f};
        constexpr Color BARK{0.231f, 0.172f, 0.105f};
        constexpr Color SUNLIGHT{0.905f, 0.823f, 0.478f};
    } // namespace Forest

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

struct PanelApp {
    std::string name;
    std::string command;
    std::string icon_label;
};

struct TrayIndicator {
    std::string label;
    std::string description;
    SwissDesign::Color color;
};

struct LauncherEntry {
    std::string name;
    std::string command;
    std::string description;
    std::string category;
};

struct ForestUIState {
    std::vector<PanelApp> panel_apps;
    std::vector<TrayIndicator> tray_icons;
    std::vector<LauncherEntry> launcher_entries;
    bool launcher_visible{false};
    std::size_t highlighted_index{0};
    std::chrono::steady_clock::time_point last_interaction{std::chrono::steady_clock::now()};
};

constexpr int FOREST_PANEL_MENU_WIDTH = 144;
constexpr int FOREST_LAUNCHER_WIDTH = 520;
constexpr int FOREST_LAUNCHER_ENTRY_HEIGHT = 68;
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

    struct wlr_cursor *cursor;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    double cursor_x;
    double cursor_y;
    bool pointer_connected;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;

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
    ForestUIState ui_state{};
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
void render_swiss_panel(cairo_t *cairo, int width, int height, float opacity, const struct ArolloaServer *server);
void render_swiss_window(cairo_t *cairo, struct ArolloaView *view, float global_opacity);
void initialize_forest_ui(struct ArolloaServer *server);

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
void setup_pointer_interactions(struct ArolloaServer *server);
void teardown_pointer_interactions(struct ArolloaServer *server);
void ensure_default_cursor(struct ArolloaServer *server);
void toggle_launcher(struct ArolloaServer *server);
void focus_launcher_offset(struct ArolloaServer *server, int offset);
bool activate_launcher_selection(struct ArolloaServer *server);
std::string get_config_string(const std::string& key, const std::string& default_value);
int get_config_int(const std::string& key, int default_value);
bool get_config_bool(const std::string& key, bool default_value);
#endif
