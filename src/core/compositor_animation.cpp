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

    for (auto &anim : server->animations) {
        if (anim && anim->active) {
            anim->update(current_time);
        }
    }

    server->animations.erase(std::remove_if(server->animations.begin(), server->animations.end(),
        [](const std::unique_ptr<Animation> &anim) {
            return !anim || !anim->active;
        }), server->animations.end());
}
