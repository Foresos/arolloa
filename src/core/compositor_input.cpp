#include "../../include/arolloa.h"

#include <cstdlib>

namespace {
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

    if (!handled) {
        wlr_seat_set_keyboard(seat, wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}
} // namespace

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
