#include "../../include/arolloa.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>

#include <wlr/render/pass.h>

namespace {
float linear_interpolate(float from, float to, float t) {
    return from + (to - from) * t;
}

struct timespec get_monotonic_time() {
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}
} // namespace

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

    const struct timespec now = get_monotonic_time();

    int width = 0;
    int height = 0;
    wlr_output_effective_resolution(output->wlr_output, &width, &height);

    const float fade = std::clamp(server->startup_opacity, 0.0f, 1.0f);
    const float background[4] = {
        linear_interpolate(SwissDesign::LIGHT_GREY.r, SwissDesign::WHITE.r, fade),
        linear_interpolate(SwissDesign::LIGHT_GREY.g, SwissDesign::WHITE.g, fade),
        linear_interpolate(SwissDesign::LIGHT_GREY.b, SwissDesign::WHITE.b, fade),
        1.0f
    };

    struct wlr_output_state state;
    wlr_output_state_init(&state);

    struct wlr_render_pass *render_pass = wlr_output_begin_render_pass(output->wlr_output, &state, nullptr, nullptr);
    if (!render_pass) {
        wlr_output_state_finish(&state);
        return;
    }

    const struct wlr_box background_box = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = height,
    };
    struct wlr_render_rect_options background_rect = {};
    background_rect.box = background_box;
    background_rect.color = {
        .r = background[0] * background[3],
        .g = background[1] * background[3],
        .b = background[2] * background[3],
        .a = background[3],
    };
    wlr_render_pass_add_rect(render_pass, &background_rect);

    const struct wlr_box panel_box = {
        .x = 0,
        .y = 0,
        .width = width,
        .height = SwissDesign::PANEL_HEIGHT
    };
    struct wlr_render_rect_options panel_rect = {};
    panel_rect.box = panel_box;
    panel_rect.color = {
        .r = SwissDesign::WHITE.r * fade,
        .g = SwissDesign::WHITE.g * fade,
        .b = SwissDesign::WHITE.b * fade,
        .a = fade,
    };
    wlr_render_pass_add_rect(render_pass, &panel_rect);

    const struct wlr_box accent_box = {
        .x = 0,
        .y = SwissDesign::PANEL_HEIGHT - SwissDesign::BORDER_WIDTH,
        .width = width,
        .height = SwissDesign::BORDER_WIDTH
    };
    struct wlr_render_rect_options accent_rect = {};
    accent_rect.box = accent_box;
    accent_rect.color = {
        .r = SwissDesign::SWISS_RED.r * fade,
        .g = SwissDesign::SWISS_RED.g * fade,
        .b = SwissDesign::SWISS_RED.b * fade,
        .a = fade,
    };
    wlr_render_pass_add_rect(render_pass, &accent_rect);

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

static void output_request_state(struct wl_listener *listener, void *data) {
    ArolloaOutput *output = wl_container_of(listener, output, request_state);
    const auto *event = static_cast<const struct wlr_output_event_request_state *>(data);
    wlr_output_commit_state(output->wlr_output, event->state);
}

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

    output->request_state.notify = output_request_state;
    wl_signal_add(&wlr_output->events.request_state, &output->request_state);

    output->destroy.notify = [](struct wl_listener *listener, void *data) {
        (void)data;
        ArolloaOutput *output = wl_container_of(listener, output, destroy);
        wl_list_remove(&output->frame.link);
        wl_list_remove(&output->request_state.link);
        wl_list_remove(&output->link);
        free(output);
    };
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    wl_list_insert(&server->outputs, &output->link);
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    wlr_log(WLR_INFO, "Registered output '%s'", wlr_output->name);
}
