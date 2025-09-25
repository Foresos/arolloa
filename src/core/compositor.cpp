#include "../../include/arolloa.h"

#include <algorithm>
#include <concepts>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <type_traits>
#include <memory>
#include <string>
#include <utility>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

// Global server instance for signal handling
static ArolloaServer *g_server = nullptr;

namespace {
constexpr float STARTUP_ANIMATION_SCALE = SwissDesign::ANIMATION_DURATION * 3.0f;
constexpr uint32_t DEFAULT_COMPOSITOR_VERSION = 5;
constexpr uint32_t DEFAULT_XDG_VERSION = 5;

void build_projection_matrix(int width, int height, float matrix[9]) {
    matrix[0] = 2.0f / static_cast<float>(width);
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;
    matrix[4] = 2.0f / static_cast<float>(height);
    matrix[5] = 0.0f;
    matrix[6] = -1.0f;
    matrix[7] = -1.0f;
    matrix[8] = 1.0f;
}

struct timespec get_monotonic_time() {
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

struct wlr_backend *autocreate_backend(struct wl_display *display) {
    if constexpr (requires(struct wl_display *d, struct wlr_session **s) {
                      { wlr_backend_autocreate(d, s) } -> std::same_as<struct wlr_backend *>;
                  }) {
        struct wlr_session *session = nullptr;
        return wlr_backend_autocreate(display, &session);
    } else {
        return wlr_backend_autocreate(display);
    }
}

struct wlr_compositor *create_compositor(struct wl_display *display, struct wlr_renderer *renderer) {
    if constexpr (requires(struct wl_display *d, uint32_t version, struct wlr_renderer *r) {
                      { wlr_compositor_create(d, version, r) } -> std::same_as<struct wlr_compositor *>;
                  }) {
        return wlr_compositor_create(display, DEFAULT_COMPOSITOR_VERSION, renderer);
    } else {
        return wlr_compositor_create(display, renderer);
    }
}

struct wlr_xdg_shell *create_xdg_shell(struct wl_display *display) {
    if constexpr (requires(struct wl_display *d, uint32_t version) {
                      { wlr_xdg_shell_create(d, version) } -> std::same_as<struct wlr_xdg_shell *>;
                  }) {
        return wlr_xdg_shell_create(display, DEFAULT_XDG_VERSION);
    } else {
        return wlr_xdg_shell_create(display);
    }
}

void push_animation(ArolloaServer *server, std::unique_ptr<Animation> animation) {
    if (!server || !animation) {
        return;
    }
    server->animations.push_back(std::move(animation));
}

void ensure_runtime_dir() {
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0] != '\0') {
        return;
    }
    const char *user = getenv("USER");
    if (!user) {
        user = "arolloa";
    }
    std::string fallback = std::string("/tmp/arolloa-runtime-") + user;
    setenv("XDG_RUNTIME_DIR", fallback.c_str(), true);
    mkdir(fallback.c_str(), 0700);
}

void schedule_startup_animation(ArolloaServer *server) {
    server->startup_opacity = 0.0f;
    auto animation = std::make_unique<Animation>();
    animation->start(0.0f, 1.0f, STARTUP_ANIMATION_SCALE, [server](float value) {
        server->startup_opacity = value;
    });
    push_animation(server, std::move(animation));
}


float linear_interpolate(float from, float to, float t) {
    return from + (to - from) * t;
}

} // namespace

void Animation::start(float from, float to, float dur, std::function<void(float)> callback) {
    const struct timespec ts = get_monotonic_time();
    start_time = ts.tv_sec + ts.tv_nsec / 1e9f;
    start_value = from;
    end_value = to;
    duration = dur;
    update_callback = std::move(callback);
    active = true;
}

void Animation::update(float current_time) {
    if (!active || duration <= 0.0f) {
        return;
    }

    float progress = (current_time - start_time) / duration;
    if (progress >= 1.0f) {
        progress = 1.0f;
        active = false;
    } else if (progress <= 0.0f) {
        progress = 0.0f;
    }

    const float eased = progress * progress * (3.0f - 2.0f * progress);
    const float value = start_value + (end_value - start_value) * eased;
    if (update_callback) {
        update_callback(value);
    }
}

static void handle_signal(int sig) {
    (void)sig;
    if (g_server && g_server->wl_display) {
        wl_display_terminate(g_server->wl_display);
    }
}

static void xdg_surface_map(struct wl_listener *listener, void *data) {
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

static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaView *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->link);
    free(view);
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    (void)listener;
    (void)data;
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    (void)listener;
    (void)data;
}

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

void server_new_output(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_output);
    auto *wlr_output = static_cast<struct wlr_output *>(data);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            wlr_log(WLR_ERROR, "Failed to commit output mode");
            return;
        }
    }

    ArolloaOutput *output = static_cast<ArolloaOutput *>(calloc(1, sizeof(ArolloaOutput)));
    if (!output) {
        return;
    }

    output->wlr_output = wlr_output;
    output->server = server;
    output->last_frame = get_monotonic_time();

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = [](struct wl_listener *listener, void *data) {
        (void)data;
        ArolloaOutput *output = wl_container_of(listener, output, destroy);
        wl_list_remove(&output->frame.link);
        wl_list_remove(&output->link);
        free(output);
    };
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    wlr_log(WLR_INFO, "Registered output '%s'", wlr_output->name);
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);
    wlr_seat_set_keyboard(keyboard->server->seat, wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    ArolloaKeyboard *keyboard = wl_container_of(listener, keyboard, key);
    ArolloaServer *server = keyboard->server;
    auto *event = static_cast<struct wlr_keyboard_key_event *>(data);
    struct wlr_seat *seat = server->seat;

    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);

    const uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms = nullptr;
    const int nsyms = xkb_state_key_get_syms(wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    const uint32_t modifiers = wlr_keyboard_get_modifiers(wlr_keyboard);
    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; ++i) {
            if (syms[i] == XKB_KEY_F4) {
                wl_display_terminate(server->wl_display);
                handled = true;
                break;
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
    auto *device = static_cast<struct wlr_input_device *>(data);

    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            ArolloaKeyboard *keyboard = static_cast<ArolloaKeyboard *>(calloc(1, sizeof(ArolloaKeyboard)));
            if (!keyboard) {
                return;
            }

            keyboard->server = server;
            keyboard->device = device;

            struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

            struct xkb_rule_names rules = {};
            rules.rules = getenv("XKB_DEFAULT_RULES");
            rules.model = getenv("XKB_DEFAULT_MODEL");
            rules.layout = getenv("XKB_DEFAULT_LAYOUT");
            rules.variant = getenv("XKB_DEFAULT_VARIANT");
            rules.options = getenv("XKB_DEFAULT_OPTIONS");

            struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

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
            break;
        default:
            break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

void render_swiss_panel(cairo_t *cairo, int width, int height, float opacity) {
    (void)height;
    cairo_set_source_rgba(cairo, SwissDesign::WHITE.r, SwissDesign::WHITE.g, SwissDesign::WHITE.b, opacity);
    cairo_rectangle(cairo, 0, 0, width, SwissDesign::PANEL_HEIGHT);
    cairo_fill(cairo);

    cairo_set_source_rgba(cairo, SwissDesign::LIGHT_GREY.r, SwissDesign::LIGHT_GREY.g, SwissDesign::LIGHT_GREY.b, opacity);
    cairo_set_line_width(cairo, SwissDesign::BORDER_WIDTH);
    cairo_move_to(cairo, 0, SwissDesign::PANEL_HEIGHT - 1);
    cairo_line_to(cairo, width, SwissDesign::PANEL_HEIGHT - 1);
    cairo_stroke(cairo);

    cairo_set_source_rgba(cairo, SwissDesign::SWISS_RED.r, SwissDesign::SWISS_RED.g, SwissDesign::SWISS_RED.b, opacity);
    cairo_select_font_face(cairo, SwissDesign::PRIMARY_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cairo, 14);
    cairo_move_to(cairo, 16, 20);
    cairo_show_text(cairo, "Arolloa");
}

void render_swiss_window(cairo_t *cairo, ArolloaView *view, float global_opacity) {
    if (!view->mapped) {
        return;
    }

    const float opacity = view->opacity * global_opacity;
    cairo_set_source_rgba(cairo, SwissDesign::WHITE.r, SwissDesign::WHITE.g, SwissDesign::WHITE.b, opacity * 0.9f);
    cairo_rectangle(cairo, view->x, view->y, 400, 300);
    cairo_fill(cairo);

    cairo_set_source_rgba(cairo, SwissDesign::GREY.r, SwissDesign::GREY.g, SwissDesign::GREY.b, opacity);
    cairo_set_line_width(cairo, SwissDesign::BORDER_WIDTH);
    cairo_rectangle(cairo, view->x, view->y, 400, 300);
    cairo_stroke(cairo);
}

void animation_tick(ArolloaServer *server) {
#ifdef __cplusplus
    if (!server) {
        return;
    }
    const struct timespec ts = get_monotonic_time();
    const float current_time = ts.tv_sec + ts.tv_nsec / 1e9f;

    for (auto &anim : server->animations) {
        if (anim && anim->active) {
            anim->update(current_time);
        }
    }

    server->animations.erase(std::remove_if(server->animations.begin(), server->animations.end(),
        [](const std::unique_ptr<Animation> &anim) {
            return !anim || !anim->active;
        }), server->animations.end());
#else
    (void)server;
#endif
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

    cairo_set_source_rgba(server->cairo_ctx, SwissDesign::WHITE.r, SwissDesign::WHITE.g, SwissDesign::WHITE.b, 1.0f);
    cairo_paint(server->cairo_ctx);

    const float opacity = std::clamp(server->startup_opacity, 0.0f, 1.0f);
    render_swiss_panel(server->cairo_ctx, width, height, opacity);

    ArolloaView *view = nullptr;
    wl_list_for_each(view, &server->views, link) {
        render_swiss_window(server->cairo_ctx, view, opacity);
    }
}

void output_frame(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaOutput *output = wl_container_of(listener, output, frame);
    ArolloaServer *server = output->server;
    struct wlr_renderer *renderer = server->renderer;

    const struct timespec now = get_monotonic_time();

    if (!wlr_output_attach_render(output->wlr_output, nullptr)) {
        return;
    }

    int width = 0;
    int height = 0;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    wlr_renderer_begin(renderer, width, height);

    const float fade = std::clamp(server->startup_opacity, 0.0f, 1.0f);
    const float background[4] = {
        linear_interpolate(SwissDesign::LIGHT_GREY.r, SwissDesign::WHITE.r, fade),
        linear_interpolate(SwissDesign::LIGHT_GREY.g, SwissDesign::WHITE.g, fade),
        linear_interpolate(SwissDesign::LIGHT_GREY.b, SwissDesign::WHITE.b, fade),
        1.0f
    };
    wlr_renderer_clear(renderer, background);

    float projection[9];
    build_projection_matrix(width, height, projection);

    const struct wlr_box panel_box = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = SwissDesign::PANEL_HEIGHT
    };
    const float panel_color[4] = {
        SwissDesign::WHITE.r,
        SwissDesign::WHITE.g,
        SwissDesign::WHITE.b,
        fade
    };
    wlr_render_rect(renderer, &panel_box, panel_color, projection);

    const struct wlr_box accent_box = {
        .x = 0,
        .y = SwissDesign::PANEL_HEIGHT - SwissDesign::BORDER_WIDTH,
        .width = width,
        .height = SwissDesign::BORDER_WIDTH
    };
    const float accent_color[4] = {
        SwissDesign::SWISS_RED.r,
        SwissDesign::SWISS_RED.g,
        SwissDesign::SWISS_RED.b,
        fade
    };
    wlr_render_rect(renderer, &accent_box, accent_color, projection);

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

        wlr_render_texture(renderer, texture, projection, box.x, box.y, alpha);

        wlr_surface_send_frame_done(surface, &now);
    }

    wlr_renderer_end(renderer);
    wlr_output_commit(output->wlr_output);
}

bool setup_debug_environment(ArolloaServer *server) {
    if (!server->debug_mode) {
        server->nested_backend_active = false;
        return true;
    }

    if (!getenv("WLR_BACKENDS")) {
        setenv("WLR_BACKENDS", "wayland", 1);
    }
    if (!getenv("WLR_WL_OUTPUTS")) {
        setenv("WLR_WL_OUTPUTS", "1", 1);
    }
    if (!getenv("WLR_RENDERER_ALLOW_SOFTWARE")) {
        setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
    }
    server->nested_backend_active = true;
    return true;
}

void server_init(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->initialized = false;
    ensure_runtime_dir();
    setup_debug_environment(server);

    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "Failed to create Wayland display");
        return;
    }

    server->backend = autocreate_backend(server->wl_display);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "Failed to create wlroots backend");
        return;
    }

    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create renderer");
        return;
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->compositor = create_compositor(server->wl_display, server->renderer);
    if (!server->compositor) {
        wlr_log(WLR_ERROR, "Failed to create compositor global");
        return;
    }

    server->xdg_shell = create_xdg_shell(server->wl_display);
    if (!server->xdg_shell) {
        wlr_log(WLR_ERROR, "Failed to create xdg-shell global");
        return;
    }

    server->decoration_manager = wlr_xdg_decoration_manager_v1_create(server->wl_display);
    server->output_layout = wlr_output_layout_create();

    wl_list_init(&server->outputs);
    wl_list_init(&server->views);
    wl_list_init(&server->keyboards);

    server->layout_mode = WindowLayout::GRID;

    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

    server->new_input.notify = server_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    server->seat = wlr_seat_create(server->wl_display, "seat0");
    server->cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
    if (server->cursor_mgr) {
        wlr_xcursor_manager_load(server->cursor_mgr, 1);
    }

    server->ui_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1920, 1080);
    server->cairo_ctx = cairo_create(server->ui_surface);
    server->pango_layout = pango_cairo_create_layout(server->cairo_ctx);
    PangoFontDescription *desc = pango_font_description_from_string((std::string(SwissDesign::PRIMARY_FONT) + " 10").c_str());
    pango_layout_set_font_description(server->pango_layout, desc);
    pango_font_description_free(desc);

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_log(WLR_ERROR, "Failed to add Wayland socket");
        return;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        return;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Running Arolloa on WAYLAND_DISPLAY=%s%s", socket,
            server->debug_mode ? " (debug nested mode)" : "");

    schedule_startup_animation(server);
    server->initialized = true;
}

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

static void launch_oobe_if_needed() {
    const char *home = getenv("HOME");
    std::string setup_file = home ? (std::string(home) + "/.config/arolloa/setup_complete") : "/tmp/arolloa_setup_complete";

    if (access(setup_file.c_str(), F_OK) == -1) {
        pid_t child = fork();
        if (child == 0) {
            execlp("./build/arolloa-oobe", "arolloa-oobe", static_cast<char *>(nullptr));
            _exit(1);
        }
    }
}

static void print_usage(const char *argv0) {
    std::fprintf(stdout, "Usage: %s [--debug] [--verbose]\n", argv0);
    std::fprintf(stdout, "  --debug    Run nested inside an existing compositor for development.\n");
    std::fprintf(stdout, "  --verbose  Enable verbose wlroots logging.\n");
}

int main(int argc, char **argv) {
    bool debug_mode = false;
    bool verbose_logging = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            verbose_logging = true;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    wlr_log_init((debug_mode || verbose_logging) ? WLR_DEBUG : WLR_INFO, nullptr);

    load_swiss_config();

    ArolloaServer server{};
    server.debug_mode = debug_mode;
    server.nested_backend_active = false;
    server.initialized = false;
    server.startup_opacity = 0.0f;

    server_init(&server);
    if (!server.initialized) {
        wlr_log(WLR_ERROR, "Failed to initialise Arolloa compositor");
        server_destroy(&server);
        return 1;
    }

    launch_oobe_if_needed();

    server_run(&server);
    server_destroy(&server);

    return 0;
}
