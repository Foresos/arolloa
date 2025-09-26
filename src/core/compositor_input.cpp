#include "../../include/arolloa.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <thread>

#include <wlr/version.h>

#include <linux/input-event-codes.h>

namespace {
using namespace std::chrono_literals;


void show_system_notification(ArolloaServer *server, const std::string &title, const std::string &body);
void show_volume_change(ArolloaServer *server, int level);
void mark_last_interaction(ArolloaServer *server) {
    if (!server) {
        return;
    }
    server->ui_state.last_interaction = std::chrono::steady_clock::now();
}

void spawn_command_async(const std::string &command) {
    if (command.empty()) {
        return;
    }

    std::thread([command]() {
        std::string wrapped = command;
        if (!wrapped.empty() && wrapped.back() != '&') {
            wrapped += " &";
        }
        std::system(wrapped.c_str());
    }).detach();
}

bool pointer_in_panel(const ArolloaServer *server) {
    return server && server->cursor_y <= static_cast<double>(SwissDesign::PANEL_HEIGHT);
}

void update_pointer_hover_state(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->ui_state.menu_hovered = false;
    server->ui_state.hovered_panel_index = -1;
    server->ui_state.hovered_tray_index = -1;

    if (!pointer_in_panel(server)) {
        return;
    }

    server->ui_state.menu_hovered = server->cursor_x <= FOREST_PANEL_MENU_WIDTH;

    const double icon_size = 28.0;
    const double spacing = 12.0;
    double local_x = server->cursor_x - FOREST_PANEL_MENU_WIDTH - spacing;
    for (std::size_t index = 0; index < server->ui_state.panel_apps.size(); ++index) {
        if (local_x >= 0.0 && local_x <= icon_size) {
            server->ui_state.hovered_panel_index = static_cast<int>(index);
            break;
        }
        local_x -= icon_size + spacing;
    }

    int width = 0;
    int height = 0;
    if (auto *output = wlr_output_layout_output_at(server->output_layout, server->cursor_x, server->cursor_y)) {
        wlr_output_effective_resolution(output, &width, &height);
    }

    if (width <= 0) {
        return;
    }

    const double tray_icon = 22.0;
    const double tray_spacing = 18.0;
    double anchor = static_cast<double>(width) - 16.0;
    for (int index = static_cast<int>(server->ui_state.tray_icons.size()) - 1; index >= 0; --index) {
        anchor -= tray_icon;
        if (server->cursor_x >= anchor && server->cursor_x <= anchor + tray_icon) {
            server->ui_state.hovered_tray_index = index;
            break;
        }
        anchor -= tray_spacing;
    }
}

void remove_listener_safe(struct wl_listener *listener) {
    if (!listener) {
        return;
    }
    if (listener->link.prev || listener->link.next) {
        wl_list_remove(&listener->link);
        listener->link.prev = nullptr;
        listener->link.next = nullptr;
    }
}

bool handle_launcher_click(ArolloaServer *server, const wlr_pointer_button_event *event) {
    if (!server->ui_state.launcher_visible || event->state != WLR_BUTTON_PRESSED) {
        return false;
    }

    struct wlr_output *output = wlr_output_layout_output_at(server->output_layout, server->cursor_x, server->cursor_y);
    if (!output) {
        server->ui_state.launcher_visible = false;
        mark_last_interaction(server);
        return true;
    }

    int width = 0;
    int height = 0;
    wlr_output_effective_resolution(output, &width, &height);

    const double launcher_width = FOREST_LAUNCHER_WIDTH;
    const double launcher_height = std::min<double>(height * 0.6,
        std::max<double>(SwissDesign::PANEL_HEIGHT * 4.0,
            server->ui_state.launcher_entries.size() * FOREST_LAUNCHER_ENTRY_HEIGHT + 96.0));

    const double start_x = (width - launcher_width) / 2.0;
    const double start_y = (height - launcher_height) / 2.0;
    const double local_x = server->cursor_x - start_x;
    const double local_y = server->cursor_y - start_y;

    if (local_x < 0.0 || local_y < 0.0 || local_x > launcher_width || local_y > launcher_height) {
        server->ui_state.launcher_visible = false;
        mark_last_interaction(server);
        return true;
    }

    if (local_y < 72.0) {
        return true;
    }

    const std::size_t index = static_cast<std::size_t>((local_y - 72.0) / FOREST_LAUNCHER_ENTRY_HEIGHT);
    if (index < server->ui_state.launcher_entries.size()) {
        server->ui_state.highlighted_index = index;
        mark_last_interaction(server);
        activate_launcher_selection(server);
    }

    return true;
}

bool handle_panel_click(ArolloaServer *server, const wlr_pointer_button_event *event) {
    if (event->state != WLR_BUTTON_PRESSED || !pointer_in_panel(server)) {
        return false;
    }

    mark_last_interaction(server);

    if (event->button != BTN_LEFT) {
        return true;
    }

    if (server->cursor_x < FOREST_PANEL_MENU_WIDTH) {
        toggle_launcher(server);
        return true;
    }

    const int hovered = server->ui_state.hovered_panel_index;
    if (hovered >= 0 && hovered < static_cast<int>(server->ui_state.panel_apps.size())) {
        const auto &app = server->ui_state.panel_apps[static_cast<std::size_t>(hovered)];
        spawn_command_async(app.command);
        show_system_notification(server, "Launching", app.name);
        return true;
    }

    return true;
}

void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);

    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(keyboard->device);
    wlr_seat_set_keyboard(keyboard->server->seat, wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat, &wlr_keyboard->modifiers);
}

void keyboard_handle_key(struct wl_listener *listener, void *data) {
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

    if (!handled && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; ++i) {
            const xkb_keysym_t sym = syms[i];
            if ((modifiers & WLR_MODIFIER_LOGO) && sym == XKB_KEY_space) {
                toggle_launcher(server);
                mark_last_interaction(server);
                handled = true;
                break;
            }

            if (server->ui_state.launcher_visible) {
                if (sym == XKB_KEY_Escape) {
                    server->ui_state.launcher_visible = false;
                    mark_last_interaction(server);
                    handled = true;
                    break;
                }
                if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter) {
                    handled = activate_launcher_selection(server);
                    break;
                }
                if (sym == XKB_KEY_Up) {
                    focus_launcher_offset(server, -1);
                    handled = true;
                    break;
                }
                if (sym == XKB_KEY_Down) {
                    focus_launcher_offset(server, 1);
                    handled = true;
                    break;
                }
            }
        }
    }

    if (!handled) {
        wlr_seat_set_keyboard(seat, wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}

void cursor_handle_motion(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, cursor_motion);
    auto *event = static_cast<struct wlr_pointer_motion_event *>(data);
    struct wlr_input_device *device = nullptr;
    if (event->pointer) {
        device = &event->pointer->base;
    }
    wlr_cursor_move(server->cursor, device, event->delta_x, event->delta_y);
    server->cursor_x = server->cursor->x;
    server->cursor_y = server->cursor->y;
    mark_last_interaction(server);
    update_pointer_hover_state(server);
    wlr_seat_pointer_notify_motion(server->seat, event->time_msec, server->cursor_x, server->cursor_y);
}

void cursor_handle_motion_absolute(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, cursor_motion_absolute);
    auto *event = static_cast<struct wlr_pointer_motion_absolute_event *>(data);
    struct wlr_input_device *device = nullptr;
    if (event->pointer) {
        device = &event->pointer->base;
    }
    wlr_cursor_warp_absolute(server->cursor, device, event->x, event->y);
    server->cursor_x = server->cursor->x;
    server->cursor_y = server->cursor->y;
    mark_last_interaction(server);
    update_pointer_hover_state(server);
    wlr_seat_pointer_notify_motion(server->seat, event->time_msec, server->cursor_x, server->cursor_y);
}

void cursor_handle_button(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, cursor_button);
    auto *event = static_cast<struct wlr_pointer_button_event *>(data);

    bool handled = handle_panel_click(server, event);
    handled = handle_launcher_click(server, event) || handled;

    if (!handled) {
        wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
    }
    mark_last_interaction(server);
}

void cursor_handle_axis(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, cursor_axis);
    auto *event = static_cast<struct wlr_pointer_axis_event *>(data);
#if defined(WLR_VERSION_NUM) && WLR_VERSION_NUM >= ((0 << 16) | (18 << 8) | 0)
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta,
                                 event->delta_discrete, event->source, event->relative_direction);
#else
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta,
                                 event->delta_discrete, event->source);
#endif

    if (pointer_in_panel(server) && server->ui_state.hovered_tray_index >= 0 &&
        server->ui_state.hovered_tray_index < static_cast<int>(server->ui_state.tray_icons.size())) {
        const auto &indicator = server->ui_state.tray_icons[static_cast<std::size_t>(server->ui_state.hovered_tray_index)];
        if (indicator.label == "VOL") {
            int delta = 0;
            if (event->delta_discrete != 0) {
                delta = static_cast<int>(event->delta_discrete);
            } else if (std::abs(event->delta) > 0.0) {
                delta = event->delta > 0.0 ? 1 : -1;
            }
            if (delta != 0) {
                const int new_level = std::clamp(server->ui_state.volume_feedback.level - delta * 2, 0, 100);
                show_volume_change(server, new_level);
            }
        }
    }

    mark_last_interaction(server);
}

void show_system_notification(ArolloaServer *server, const std::string &title, const std::string &body) {
    if (!server) {
        return;
    }

    if (!server->ui_state.notifications_enabled) {
        return;
    }

    ForestUIState::Notification notification;
    notification.title = title;
    notification.body = body;
    notification.accent = server->ui_state.accent_color;
    notification.opacity = 0.0f;
    notification.target_opacity = 1.0f;
    notification.created = std::chrono::steady_clock::now();
    server->ui_state.notifications.emplace_back(std::move(notification));
    if (server->ui_state.notifications.size() > 6) {
        server->ui_state.notifications.erase(server->ui_state.notifications.begin());
    }
}

void show_volume_change(ArolloaServer *server, int level) {
    if (!server) {
        return;
    }

    level = std::clamp(level, 0, 100);
    server->ui_state.volume_feedback.level = level;
    server->ui_state.volume_feedback.target_visibility = server->ui_state.notifications_enabled ? 1.0f : 0.0f;
    server->ui_state.volume_feedback.last_update = std::chrono::steady_clock::now();

    if (!server->ui_state.notifications_enabled) {
        return;
    }

    server->ui_state.notifications.erase(std::remove_if(server->ui_state.notifications.begin(),
            server->ui_state.notifications.end(),
        [](const ForestUIState::Notification &notification) {
            return notification.is_volume;
        }), server->ui_state.notifications.end());

    ForestUIState::Notification notification;
    notification.title = "Volume";
    notification.body = std::to_string(level) + "%";
    notification.accent = server->ui_state.accent_color;
    notification.opacity = 0.0f;
    notification.target_opacity = 1.0f;
    notification.created = server->ui_state.volume_feedback.last_update;
    notification.is_volume = true;
    notification.volume_level = level;
    server->ui_state.notifications.emplace_back(std::move(notification));
}

void cursor_handle_frame(struct wl_listener *listener, void *data) {
    (void)data;
    ArolloaServer *server = wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

void seat_handle_request_cursor(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, request_cursor);
    auto *event = static_cast<struct wlr_seat_pointer_request_set_cursor_event *>(data);

    if (event->seat_client == server->seat->pointer_state.focused_client) {
        wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
    }
}

void seat_handle_set_selection(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, request_set_selection);
    auto *event = static_cast<struct wlr_seat_request_set_selection_event *>(data);
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

} // namespace

void ensure_default_cursor(ArolloaServer *server) {
    if (!server || !server->cursor) {
        return;
    }

    if (server->cursor_mgr) {
        if (auto *xcursor = wlr_xcursor_manager_get_xcursor(server->cursor_mgr, "left_ptr", 1.0f)) {
            (void)xcursor;
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
            return;
        }
    }

}

void toggle_launcher(ArolloaServer *server) {
    if (!server) {
        return;
    }

    if (server->ui_state.launcher_entries.empty()) {
        server->ui_state.launcher_visible = false;
        return;
    }

    server->ui_state.launcher_visible = !server->ui_state.launcher_visible;
    if (server->ui_state.highlighted_index >= server->ui_state.launcher_entries.size()) {
        server->ui_state.highlighted_index = 0;
    }
    mark_last_interaction(server);
}

void focus_launcher_offset(ArolloaServer *server, int offset) {
    if (!server || server->ui_state.launcher_entries.empty()) {
        return;
    }

    const int count = static_cast<int>(server->ui_state.launcher_entries.size());
    int index = static_cast<int>(server->ui_state.highlighted_index);
    index = (index + offset) % count;
    if (index < 0) {
        index += count;
    }
    server->ui_state.highlighted_index = static_cast<std::size_t>(index);
    mark_last_interaction(server);
}

bool activate_launcher_selection(ArolloaServer *server) {
    if (!server || server->ui_state.launcher_entries.empty()) {
        return false;
    }

    const auto &entry = server->ui_state.launcher_entries[server->ui_state.highlighted_index];
    spawn_command_async(entry.command);
    show_system_notification(server, "Launching", entry.name);
    server->ui_state.launcher_visible = false;
    mark_last_interaction(server);
    return true;
}

void setup_pointer_interactions(ArolloaServer *server) {
    if (!server || !server->cursor) {
        return;
    }

    server->cursor_motion.notify = cursor_handle_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = cursor_handle_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute);

    server->cursor_button.notify = cursor_handle_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    server->cursor_axis.notify = cursor_handle_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);

    server->cursor_frame.notify = cursor_handle_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

    server->request_cursor.notify = seat_handle_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);

    server->request_set_selection.notify = seat_handle_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);
}

void teardown_pointer_interactions(ArolloaServer *server) {
    if (!server) {
        return;
    }

    remove_listener_safe(&server->cursor_motion);
    remove_listener_safe(&server->cursor_motion_absolute);
    remove_listener_safe(&server->cursor_button);
    remove_listener_safe(&server->cursor_axis);
    remove_listener_safe(&server->cursor_frame);
    remove_listener_safe(&server->request_cursor);
    remove_listener_safe(&server->request_set_selection);
}

void server_new_input(struct wl_listener *listener, void *data) {
    ArolloaServer *server = wl_container_of(listener, server, new_input);
    auto *device = static_cast<struct wlr_input_device *>(data);

    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD: {
            auto *keyboard = static_cast<ArolloaKeyboard *>(calloc(1, sizeof(ArolloaKeyboard)));
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
        case WLR_INPUT_DEVICE_POINTER: {
            if (server->cursor) {
                wlr_cursor_attach_input_device(server->cursor, device);
            }
            server->pointer_connected = true;
            ensure_default_cursor(server);
            break;
        }
        default:
            break;
    }

    uint32_t caps = 0;
    if (server->pointer_connected) {
        caps |= WL_SEAT_CAPABILITY_POINTER;
    }
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}
