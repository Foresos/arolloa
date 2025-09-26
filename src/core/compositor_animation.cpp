#include "../../include/arolloa.h"

#include <algorithm>
#include <ctime>
#include <memory>
#include <utility>

namespace {
constexpr float STARTUP_ANIMATION_SCALE = SwissDesign::ANIMATION_DURATION * 3.0f;
}

void Animation::start(float from, float to, float dur, std::function<void(float)> callback) {
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
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

void push_animation(ArolloaServer *server, std::unique_ptr<Animation> animation) {
    if (!server || !animation) {
        return;
    }

    server->animations.push_back(std::move(animation));
}

void schedule_startup_animation(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->startup_opacity = 0.0f;
    auto animation = std::make_unique<Animation>();
    animation->start(0.0f, 1.0f, STARTUP_ANIMATION_SCALE, [server](float value) {
        server->startup_opacity = value;
    });
    push_animation(server, std::move(animation));
}

void animation_tick(ArolloaServer *server) {
    if (!server) {
        return;
    }

    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const float current_time = ts.tv_sec + ts.tv_nsec / 1e9f;

    const auto now = std::chrono::steady_clock::now();
    const float delta = std::chrono::duration<float>(now - server->ui_state.last_animation_tick).count();
    server->ui_state.last_animation_tick = now;

    const auto smooth_step = [delta](float &value, float target, float speed) {
        const float step = std::clamp(speed * delta, 0.0f, 1.0f);
        value += (target - value) * step;
        value = std::clamp(value, 0.0f, 1.0f);
    };

    smooth_step(server->ui_state.menu_hover_progress, server->ui_state.menu_hovered ? 1.0f : 0.0f, 9.5f);
    smooth_step(server->ui_state.panel_hover_progress, server->ui_state.hovered_panel_index >= 0 ? 1.0f : 0.0f, 7.5f);
    smooth_step(server->ui_state.tray_hover_progress, server->ui_state.hovered_tray_index >= 0 ? 1.0f : 0.0f, 7.5f);

    if (std::chrono::duration<float>(now - server->ui_state.volume_feedback.last_update).count() > 1.6f) {
        server->ui_state.volume_feedback.target_visibility = 0.0f;
    }
    smooth_step(server->ui_state.volume_feedback.visibility, server->ui_state.volume_feedback.target_visibility, 8.0f);

    for (auto &anim : server->animations) {
        if (anim && anim->active) {
            anim->update(current_time);
        }
    }

    server->animations.erase(std::remove_if(server->animations.begin(), server->animations.end(),
        [](const std::unique_ptr<Animation> &anim) {
            return !anim || !anim->active;
        }), server->animations.end());

    for (auto &notification : server->ui_state.notifications) {
        const float age = std::chrono::duration<float>(now - notification.created).count();
        if (age > notification.lifetime) {
            notification.target_opacity = 0.0f;
        }
        const float speed = notification.is_volume ? 10.0f : 6.0f;
        const float step = std::clamp(speed * delta, 0.0f, 1.0f);
        notification.opacity += (notification.target_opacity - notification.opacity) * step;
        notification.opacity = std::clamp(notification.opacity, 0.0f, 1.0f);
    }

    server->ui_state.notifications.erase(std::remove_if(server->ui_state.notifications.begin(), server->ui_state.notifications.end(),
        [](const ForestUIState::Notification &notification) {
            return notification.opacity <= 0.02f && notification.target_opacity <= 0.0f;
        }), server->ui_state.notifications.end());
}
